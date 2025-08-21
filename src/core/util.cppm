module;

#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

export module hsh.core.util;

export namespace hsh::core::util {

template<typename Iterator, typename Sentinel, typename CharT = char>
struct JoinView : std::ranges::subrange<Iterator, Sentinel> {
  std::basic_string_view<CharT> separator_;

  constexpr JoinView(Iterator first, Sentinel last, std::basic_string_view<CharT> sep)
      : std::ranges::subrange<Iterator, Sentinel>(first, last), separator_(sep) {}

  template<std::ranges::input_range Range>
  constexpr JoinView(Range&& range, std::basic_string_view<CharT> sep)
      : std::ranges::subrange<Iterator, Sentinel>(std::ranges::begin(range), std::ranges::end(range))
      , separator_(sep) {}
};

template<std::ranges::input_range Range, typename CharT = char>
[[nodiscard]] constexpr auto join(Range&& range, std::basic_string_view<CharT> separator = " ")
    -> JoinView<std::ranges::iterator_t<Range>, std::ranges::sentinel_t<Range>, CharT> {
  return JoinView<std::ranges::iterator_t<Range>, std::ranges::sentinel_t<Range>, CharT>{
      std::ranges::begin(range), std::ranges::end(range), separator
  };
}

template<typename Iterator, typename Sentinel, typename CharT = char>
[[nodiscard]] constexpr auto join(Iterator first, Sentinel last, std::basic_string_view<CharT> separator = " ")
    -> JoinView<Iterator, Sentinel, CharT> {
  return JoinView<Iterator, Sentinel, CharT>{first, last, separator};
}

template<std::ranges::input_range Range, typename CharT = char>
[[nodiscard]] auto join_to_string(Range&& range, std::basic_string_view<CharT> separator = " ")
    -> std::basic_string<CharT> {
  if (std::ranges::empty(range)) {
    return {};
  }

  auto it        = std::ranges::begin(range);
  auto formatted = std::format("{}", *it);
  ++it;

  for (; it != std::ranges::end(range); ++it) {
    formatted += separator;
    formatted += std::format("{}", *it);
  }

  return formatted;
}

} // namespace hsh::core::util

template<typename Iterator, typename Sentinel, typename CharT>
struct std::formatter<hsh::core::util::JoinView<Iterator, Sentinel, CharT>, CharT>
    : std::formatter<std::remove_cvref_t<std::iter_reference_t<Iterator>>, CharT> {
  template<class FormatContext>
  auto format(hsh::core::util::JoinView<Iterator, Sentinel, CharT> const& view, FormatContext& ctx) const {
    using Elem = std::remove_cvref_t<std::iter_reference_t<Iterator>>;
    auto out   = ctx.out();
    auto it    = view.begin();
    auto se    = view.end();

    if (it == se) {
      return out;
    }

    out = std::formatter<Elem, CharT>::format(*it, ctx);
    ++it;

    for (; it != se; ++it) {
      for (CharT const c : view.separator_) {
        *out++ = c;
      }
      out = std::formatter<Elem, CharT>::format(*it, ctx);
    }
    return out;
  }
};
