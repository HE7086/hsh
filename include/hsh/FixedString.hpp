#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>
#include <format>
#include <string_view>
#include <type_traits>

namespace hsh {

// NOLINTBEGIN

template<size_t N, typename CharT = char>
  requires std::is_nothrow_assignable_v<CharT&, CharT const&>
struct FixedString {
  std::array<CharT, N> data{};

  consteval FixedString(CharT const (&str)[N]) noexcept {
    std::copy_n(str, N, data.begin());
  }
  constexpr operator std::basic_string_view<CharT>() const noexcept {
    return {data.data(), N - 1};
  }
  constexpr operator CharT const*() const noexcept {
    return data.data();
  }
};
template<size_t N, typename CharT>
FixedString(CharT const (&)[N]) -> FixedString<N, CharT>;

// NOLINTEND

}; // namespace hsh

template<typename CharT, size_t N>
struct std::formatter<hsh::FixedString<N, CharT>, CharT> : std::formatter<std::basic_string_view<CharT>, CharT> {
  template<typename FormatContext>
  auto format(hsh::FixedString<N, CharT> const& fs, FormatContext& ctx) const {
    return std::formatter<std::basic_string_view<CharT>, CharT>::format(fs, ctx);
  }
};
