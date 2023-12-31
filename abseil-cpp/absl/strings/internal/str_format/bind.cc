#include "absl/strings/internal/str_format/bind.h"

#include <cerrno>
#include <limits>
#include <sstream>
#include <string>

namespace absl {
namespace str_format_internal {

namespace {

inline bool BindFromPosition(int position, int* value,
                             absl::Span<const FormatArgImpl> pack) {
  assert(position > 0);
  if (static_cast<size_t>(position) > pack.size()) {
    return false;
  }
  // -1 because positions are 1-based
  return FormatArgImplFriend::ToInt(pack[position - 1], value);
}

class ArgContext {
 public:
  explicit ArgContext(absl::Span<const FormatArgImpl> pack) : pack_(pack) {}

  // Fill 'bound' with the results of applying the context's argument pack
  // to the specified 'props'. We synthesize a BoundConversion by
  // lining up a UnboundConversion with a user argument. We also
  // resolve any '*' specifiers for width and precision, so after
  // this call, 'bound' has all the information it needs to be formatted.
  // Returns false on failure.
  bool Bind(const UnboundConversion *props, BoundConversion *bound);

 private:
  absl::Span<const FormatArgImpl> pack_;
};

inline bool ArgContext::Bind(const UnboundConversion* unbound,
                             BoundConversion* bound) {
  const FormatArgImpl* arg = nullptr;
  int arg_position = unbound->arg_position;
  if (static_cast<size_t>(arg_position - 1) >= pack_.size()) return false;
  arg = &pack_[arg_position - 1];  // 1-based

  if (!unbound->flags.basic) {
    int width = unbound->width.value();
    bool force_left = false;
    if (unbound->width.is_from_arg()) {
      if (!BindFromPosition(unbound->width.get_from_arg(), &width, pack_))
        return false;
      if (width < 0) {
        // "A negative field width is taken as a '-' flag followed by a
        // positive field width."
        force_left = true;
        width = -width;
      }
    }

    int precision = unbound->precision.value();
    if (unbound->precision.is_from_arg()) {
      if (!BindFromPosition(unbound->precision.get_from_arg(), &precision,
                            pack_))
        return false;
    }

    bound->set_width(width);
    bound->set_precision(precision);
    bound->set_flags(unbound->flags);
    if (force_left)
      bound->set_left(true);
  } else {
    bound->set_flags(unbound->flags);
    bound->set_width(-1);
    bound->set_precision(-1);
  }

  bound->set_length_mod(unbound->length_mod);
  bound->set_conv(unbound->conv);
  bound->set_arg(arg);
  return true;
}

template <typename Converter>
class ConverterConsumer {
 public:
  ConverterConsumer(Converter converter, absl::Span<const FormatArgImpl> pack)
      : converter_(converter), arg_context_(pack) {}

  bool Append(string_view s) {
    converter_.Append(s);
    return true;
  }
  bool ConvertOne(const UnboundConversion& conv, string_view conv_string) {
    BoundConversion bound;
    if (!arg_context_.Bind(&conv, &bound)) return false;
    return converter_.ConvertOne(bound, conv_string);
  }

 private:
  Converter converter_;
  ArgContext arg_context_;
};

template <typename Converter>
bool ConvertAll(const UntypedFormatSpecImpl& format,
                absl::Span<const FormatArgImpl> args,
                const Converter& converter) {
  const ParsedFormatBase* pc = format.parsed_conversion();
  if (pc)
    return pc->ProcessFormat(ConverterConsumer<Converter>(converter, args));

  return ParseFormatString(format.str(),
                           ConverterConsumer<Converter>(converter, args));
}

class DefaultConverter {
 public:
  explicit DefaultConverter(FormatSinkImpl* sink) : sink_(sink) {}

  void Append(string_view s) const { sink_->Append(s); }

  bool ConvertOne(const BoundConversion& bound, string_view /*conv*/) const {
    return FormatArgImplFriend::Convert(*bound.arg(), bound, sink_);
  }

 private:
  FormatSinkImpl* sink_;
};

class SummarizingConverter {
 public:
  explicit SummarizingConverter(FormatSinkImpl* sink) : sink_(sink) {}

  void Append(string_view s) const { sink_->Append(s); }

  bool ConvertOne(const BoundConversion& bound, string_view /*conv*/) const {
    UntypedFormatSpecImpl spec("%d");

    std::ostringstream ss;
    ss << "{" << Streamable(spec, {*bound.arg()}) << ":" << bound.flags();
    if (bound.width() >= 0) ss << bound.width();
    if (bound.precision() >= 0) ss << "." << bound.precision();
    ss << bound.length_mod() << bound.conv() << "}";
    Append(ss.str());
    return true;
  }

 private:
  FormatSinkImpl* sink_;
};

}  // namespace

bool BindWithPack(const UnboundConversion* props,
                  absl::Span<const FormatArgImpl> pack,
                  BoundConversion* bound) {
  return ArgContext(pack).Bind(props, bound);
}

std::string Summarize(const UntypedFormatSpecImpl& format,
                 absl::Span<const FormatArgImpl> args) {
  typedef SummarizingConverter Converter;
  std::string out;
  {
    // inner block to destroy sink before returning out. It ensures a last
    // flush.
    FormatSinkImpl sink(&out);
    if (!ConvertAll(format, args, Converter(&sink))) {
      sink.Flush();
      out.clear();
    }
  }
  return out;
}

bool FormatUntyped(FormatRawSinkImpl raw_sink,
                   const UntypedFormatSpecImpl& format,
                   absl::Span<const FormatArgImpl> args) {
  FormatSinkImpl sink(raw_sink);
  using Converter = DefaultConverter;
  if (!ConvertAll(format, args, Converter(&sink))) {
    sink.Flush();
    return false;
  }
  return true;
}

std::ostream& Streamable::Print(std::ostream& os) const {
  if (!FormatUntyped(&os, format_, args_)) os.setstate(std::ios::failbit);
  return os;
}

std::string& AppendPack(std::string* out, const UntypedFormatSpecImpl& format,
                   absl::Span<const FormatArgImpl> args) {
  size_t orig = out->size();
  if (!FormatUntyped(out, format, args)) out->resize(orig);
  return *out;
}

int FprintF(std::FILE* output, const UntypedFormatSpecImpl& format,
            absl::Span<const FormatArgImpl> args) {
  FILERawSink sink(output);
  if (!FormatUntyped(&sink, format, args)) {
    errno = EINVAL;
    return -1;
  }
  if (sink.error()) {
    errno = sink.error();
    return -1;
  }
  if (sink.count() > std::numeric_limits<int>::max()) {
    errno = EFBIG;
    return -1;
  }
  return static_cast<int>(sink.count());
}

int SnprintF(char* output, size_t size, const UntypedFormatSpecImpl& format,
             absl::Span<const FormatArgImpl> args) {
  BufferRawSink sink(output, size ? size - 1 : 0);
  if (!FormatUntyped(&sink, format, args)) {
    errno = EINVAL;
    return -1;
  }
  size_t total = sink.total_written();
  if (size) output[std::min(total, size - 1)] = 0;
  return static_cast<int>(total);
}

}  // namespace str_format_internal
}  // namespace absl
