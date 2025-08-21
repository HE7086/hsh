#include <array>
#include <string>

#include <gtest/gtest.h>

import hsh.cli;

namespace hsh::cli::test {

class ArgumentParserTest : public ::testing::Test {
protected:
  void SetUp() override {
    parser_ = std::make_unique<hsh::cli::ArgumentParser>("test", "Test parser");
  }

  void TearDown() override {
    parser_.reset();
  }

  std::unique_ptr<hsh::cli::ArgumentParser> parser_;
};

TEST_F(ArgumentParserTest, FlagOption) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");
  parser_->add_argument("help", "h").desc("Show help");

  auto argv   = std::array<char const*, 3>{"test", "--verbose", "--help"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("verbose"));
  EXPECT_TRUE(result->has("help"));
  EXPECT_EQ(result->get<std::string>("verbose"), "true");
  EXPECT_EQ(result->get<std::string>("help"), "true");
}

TEST_F(ArgumentParserTest, ShortOption) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");
  parser_->add_argument("debug", "d").desc("Enable debug mode");

  auto argv   = std::array<char const*, 3>{"test", "-v", "-d"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("verbose"));
  EXPECT_TRUE(result->has("debug"));
  EXPECT_EQ(result->get<std::string>("verbose"), "true");
  EXPECT_EQ(result->get<std::string>("debug"), "true");
}

TEST_F(ArgumentParserTest, LongOption) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");
  parser_->add_argument("quiet", "q").desc("Quiet mode");

  auto argv   = std::array<char const*, 3>{"test", "--verbose", "--quiet"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("verbose"));
  EXPECT_TRUE(result->has("quiet"));
  EXPECT_EQ(result->get<std::string>("verbose"), "true");
  EXPECT_EQ(result->get<std::string>("quiet"), "true");
}

TEST_F(ArgumentParserTest, PositionalArguments) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");

  auto argv   = std::array<char const*, 6>{"test", "file1", "file2", "file3", "--verbose", "file4"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  auto positional = result->positional_;
  EXPECT_EQ(positional.size(), 4);
  EXPECT_EQ(positional[0], "file1");
  EXPECT_EQ(positional[1], "file2");
  EXPECT_EQ(positional[2], "file3");
  EXPECT_EQ(positional[3], "file4");
  EXPECT_TRUE(result->has("verbose"));
}

TEST_F(ArgumentParserTest, OptionWithArguments) {
  parser_->add_argument("output", "o").nargs(1).desc("Output file");
  parser_->add_argument("include", "I").nargs(2).desc("Include directories");

  auto argv   = std::array<char const*, 6>{"test", "-o", "output.txt", "--include", "dir1", "dir2"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("output"));
  EXPECT_TRUE(result->has("include"));
  EXPECT_EQ(result->get<std::string>("output"), "output.txt");
}

TEST_F(ArgumentParserTest, UnknownOption) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");

  auto argv   = std::array<char const*, 2>{"test", "--unknown"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Unknown option: --unknown");
}

TEST_F(ArgumentParserTest, UnknownShortOption) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");

  auto argv   = std::array<char const*, 2>{"test", "-x"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Unknown option: -x");
}

TEST_F(ArgumentParserTest, MissingArguments) {
  parser_->add_argument("output", "o").nargs(2).desc("Output files");

  auto argv   = std::array<char const*, 3>{"test", "-o", "file1"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Option -o requires 2 arguments, got 1");
}

TEST_F(ArgumentParserTest, FlagWithValue) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");

  auto argv   = std::array<char const*, 2>{"test", "--verbose=true"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Flag option --verbose does not accept a value");
}

TEST_F(ArgumentParserTest, DefaultValues) {
  parser_->add_argument("output", "o").nargs(1).default_value("default.txt").desc("Output file");
  parser_->add_argument("verbose", "v").desc("Enable verbose output");

  auto argv   = std::array<char const*, 2>{"test", "-v"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("output"));
  EXPECT_TRUE(result->has("verbose"));
  EXPECT_EQ(result->get<std::string>("output"), "default.txt");
  EXPECT_EQ(result->get<std::string>("verbose"), "true");
}

TEST_F(ArgumentParserTest, CombinedShortOptions) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");
  parser_->add_argument("help", "h").desc("Show help");
  parser_->add_argument("debug", "d").desc("Enable debug mode");

  auto argv   = std::array<char const*, 2>{"test", "-vhd"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("verbose"));
  EXPECT_TRUE(result->has("help"));
  EXPECT_TRUE(result->has("debug"));
  EXPECT_EQ(result->get<std::string>("verbose"), "true");
  EXPECT_EQ(result->get<std::string>("help"), "true");
  EXPECT_EQ(result->get<std::string>("debug"), "true");
}

TEST_F(ArgumentParserTest, CombinedShortOptionsWithArgument) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");
  parser_->add_argument("output", "o").nargs(1).desc("Output file");

  auto argv   = std::array<char const*, 3>{"test", "-vo", "file.txt"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
}


TEST_F(ArgumentParserTest, LongOptionWithEquals) {
  parser_->add_argument("output", "o").nargs(1).desc("Output file");
  parser_->add_argument("count", "c").nargs(1).desc("Number of items");

  auto argv   = std::array<char const*, 3>{"test", "--output=file.txt", "--count=42"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("output"));
  EXPECT_TRUE(result->has("count"));
  EXPECT_EQ(result->get<std::string>("output"), "file.txt");
  EXPECT_EQ(result->get<std::string>("count"), "42");
}

TEST_F(ArgumentParserTest, MixedArgumentTypes) {
  parser_->add_argument("verbose", "v").desc("Enable verbose output");
  parser_->add_argument("output", "o").nargs(1).desc("Output file");
  parser_->add_argument("threads", "t").nargs(1).default_value("1").desc("Number of threads");

  auto argv = std::array<char const*, 9>{
      "test", "input1.txt", "-v", "--output", "result.txt", "input2.txt", "-t", "4", "input3.txt"
  };
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("verbose"));
  EXPECT_TRUE(result->has("output"));
  EXPECT_TRUE(result->has("threads"));

  auto positional = result->positional_;
  EXPECT_EQ(positional.size(), 3);
  EXPECT_EQ(positional[0], "input1.txt");
  EXPECT_EQ(positional[1], "input2.txt");
  EXPECT_EQ(positional[2], "input3.txt");

  EXPECT_EQ(result->get<std::string>("output"), "result.txt");
  EXPECT_EQ(result->get<std::string>("threads"), "4");
  EXPECT_EQ(result->get<std::string>("verbose"), "true");
}

TEST_F(ArgumentParserTest, LongOptionMissingArguments) {
  parser_->add_argument("output", "o").nargs(1).desc("Output file");

  auto argv   = std::array<char const*, 2>{"test", "--output"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Option --output requires 1 arguments, got 0");
}

TEST_F(ArgumentParserTest, ShortOptionMissingArguments) {
  parser_->add_argument("count", "c").nargs(2).desc("Count values");

  auto argv   = std::array<char const*, 2>{"test", "-c"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Option -c requires 2 arguments, got 0");
}

TEST_F(ArgumentParserTest, PartialArgumentsProvided) {
  parser_->add_argument("range", "r").nargs(3).desc("Range values");

  auto argv   = std::array<char const*, 4>{"test", "-r", "1", "2"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Option -r requires 3 arguments, got 2");
}

TEST_F(ArgumentParserTest, ArgumentsFollowedByOption) {
  parser_->add_argument("files", "f").nargs(2).desc("Input files");
  parser_->add_argument("verbose", "v").desc("Verbose mode");

  auto argv   = std::array<char const*, 4>{"test", "-f", "file1.txt", "-v"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Option -f requires 2 arguments, got 1");
}

TEST_F(ArgumentParserTest, EmptyShortName) {
  parser_->add_argument("verbose", "").desc("Verbose mode");

  auto argv   = std::array<char const*, 2>{"test", "-"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  auto positional = result->positional_;
  EXPECT_EQ(positional.size(), 1);
  EXPECT_EQ(positional[0], "-");
}

TEST_F(ArgumentParserTest, DoubleHyphenOnly) {
  parser_->add_argument("verbose", "v").desc("Verbose mode");

  auto argv   = std::array<char const*, 2>{"test", "--"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Unknown option: --");
}

TEST_F(ArgumentParserTest, LongOptionEqualsButNoValue) {
  parser_->add_argument("output", "o").nargs(1).desc("Output file");

  auto argv   = std::array<char const*, 2>{"test", "--output="};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has("output"));
  EXPECT_EQ(result->get<std::string>("output"), "");
}

TEST_F(ArgumentParserTest, MultipleUnknownOptions) {
  parser_->add_argument("verbose", "v").desc("Verbose mode");

  auto argv   = std::array<char const*, 3>{"test", "--unknown1", "--unknown2"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Unknown option: --unknown1");
}

TEST_F(ArgumentParserTest, CombinedShortOptionsWithUnknown) {
  parser_->add_argument("verbose", "v").desc("Verbose mode");
  parser_->add_argument("help", "h").desc("Help");

  auto argv   = std::array<char const*, 2>{"test", "-vxh"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Unknown option: -x");
}

TEST_F(ArgumentParserTest, ArgumentsStopAtNextOption) {
  parser_->add_argument("include", "I").nargs(3).desc("Include paths");
  parser_->add_argument("verbose", "v").desc("Verbose mode");

  auto argv   = std::array<char const*, 4>{"test", "-I", "path1", "--verbose"};
  auto result = parser_->parse(argv.size(), argv.data());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Option -I requires 3 arguments, got 1");
}

} // namespace hsh::cli::test
