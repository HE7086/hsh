module;

#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module hsh.core;

export import hsh.core.constant;
export import hsh.core.syscall;
export import hsh.core.file_descriptor;

export namespace hsh::core {

namespace locale {

auto current() noexcept -> std::locale const&;

bool is_alpha(char c) noexcept;
bool is_digit(char c) noexcept;
bool is_alnum(char c) noexcept;
bool is_space(char c) noexcept;
bool is_upper(char c) noexcept;
bool is_lower(char c) noexcept;
char to_upper(char c) noexcept;
char to_lower(char c) noexcept;
bool is_alpha_u(char c) noexcept;
bool is_alnum_u(char c) noexcept;

} // namespace locale

namespace env {

auto get(std::string const& name) -> std::optional<std::string_view>;
auto list() -> std::vector<std::pair<std::string_view, std::string_view>>;
void set(std::string name, std::string value);
void unset(std::string const& name);
void populate_cache();
void export_shell(int argc, char const** argv);
auto environ() -> char**;

} // namespace env

template<typename T, typename U = std::string>
using Result = std::expected<T, U>;

namespace util {

template<typename Iterator, typename Sentinel, typename CharT = char>
struct JoinView : std::ranges::subrange<Iterator, Sentinel> {
  std::basic_string_view<CharT> separator;

  constexpr JoinView(Iterator first, Sentinel last, std::basic_string_view<CharT> sep)
      : std::ranges::subrange<Iterator, Sentinel>(first, last), separator(sep) {}

  template<std::ranges::input_range Range>
  constexpr JoinView(Range&& range, std::basic_string_view<CharT> sep)
      : std::ranges::subrange<Iterator, Sentinel>(std::ranges::begin(range), std::ranges::end(range)), separator(sep) {}
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

} // namespace util

} // namespace hsh::core

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
      for (CharT const c : view.separator) {
        *out++ = c;
      }
      out = std::formatter<Elem, CharT>::format(*it, ctx);
    }
    return out;
  }
};
