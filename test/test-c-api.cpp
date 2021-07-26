// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <gtest/gtest.h>
#include <quick-lint-js/c-api.h>
#include <quick-lint-js/char8.h>
#include <quick-lint-js/file.h>
#include <quick-lint-js/filesystem-test.h>

namespace quick_lint_js {
namespace {
class test_c_api_vscode : public ::testing::Test, protected filesystem_test {};

TEST_F(test_c_api_vscode, empty_document_has_no_diagnostics) {
  std::string project_dir = this->make_temporary_directory();
  std::string js_file = project_dir + "/test.js";

  qljs_vscode_workspace* workspace = qljs_vscode_create_workspace();
  qljs_vscode_document* p =
      qljs_vscode_create_source_document(workspace, js_file.c_str());

  const qljs_vscode_diagnostic* diagnostics = qljs_vscode_lint(p);
  EXPECT_EQ(diagnostics[0].message, nullptr);

  qljs_vscode_destroy_document(p);
  qljs_vscode_destroy_workspace(workspace);
}

TEST(test_c_api_web_demo, empty_document_has_no_diagnostics) {
  qljs_web_demo_document* p = qljs_web_demo_create_document();
  const qljs_web_demo_diagnostic* diagnostics = qljs_web_demo_lint(p);
  EXPECT_EQ(diagnostics[0].message, nullptr);
  qljs_web_demo_destroy_document(p);
}

TEST_F(test_c_api_vscode, lint_error_after_text_insertion) {
  std::string project_dir = this->make_temporary_directory();
  std::string js_file = project_dir + "/test.js";

  qljs_vscode_workspace* workspace = qljs_vscode_create_workspace();
  qljs_vscode_document* p =
      qljs_vscode_create_source_document(workspace, js_file.c_str());

  const char8* document_text = u8"let x;let x;";
  qljs_vscode_replace_text(p, /*start_line=*/0, /*start_character=*/0,
                           /*end_line=*/1, /*end_character=*/0, document_text,
                           strlen(document_text));
  const qljs_vscode_diagnostic* diagnostics = qljs_vscode_lint(p);
  EXPECT_NE(diagnostics[0].message, nullptr);
  EXPECT_EQ(diagnostics[1].message, nullptr);
  EXPECT_EQ(diagnostics[1].code, nullptr);

  EXPECT_STREQ(diagnostics[0].message, "redeclaration of variable: x");
  EXPECT_STREQ(diagnostics[0].code, "E034");
  EXPECT_EQ(diagnostics[0].start_line, 0);
  EXPECT_EQ(diagnostics[0].start_character, strlen(u8"let x;let "));
  EXPECT_EQ(diagnostics[0].end_line, 0);
  EXPECT_EQ(diagnostics[0].end_character, strlen(u8"let x;let x"));

  qljs_vscode_destroy_document(p);
  qljs_vscode_destroy_workspace(workspace);
}

TEST(test_c_api_web_demo, lint_error_after_text_insertion) {
  qljs_web_demo_document* p = qljs_web_demo_create_document();

  const char8* document_text = u8"let x;let x;";
  qljs_web_demo_set_text(p, document_text, strlen(document_text));
  const qljs_web_demo_diagnostic* diagnostics = qljs_web_demo_lint(p);
  EXPECT_NE(diagnostics[0].message, nullptr);
  EXPECT_EQ(diagnostics[1].message, nullptr);
  EXPECT_EQ(diagnostics[1].code, nullptr);

  EXPECT_STREQ(diagnostics[0].message, "redeclaration of variable: x");
  EXPECT_STREQ(diagnostics[0].code, "E034");
  EXPECT_EQ(diagnostics[0].begin_offset, strlen(u8"let x;let "));
  EXPECT_EQ(diagnostics[0].end_offset, strlen(u8"let x;let x"));

  qljs_web_demo_destroy_document(p);
}

TEST_F(test_c_api_vscode, lint_new_error_after_second_text_insertion) {
  std::string project_dir = this->make_temporary_directory();
  std::string js_file = project_dir + "/test.js";

  qljs_vscode_workspace* workspace = qljs_vscode_create_workspace();
  qljs_vscode_document* p =
      qljs_vscode_create_source_document(workspace, js_file.c_str());

  const char8* document_text = u8"let x;";
  qljs_vscode_replace_text(p, /*start_line=*/0, /*start_character=*/0,
                           /*end_line=*/1, /*end_character=*/0, document_text,
                           strlen(document_text));
  const qljs_vscode_diagnostic* diagnostics = qljs_vscode_lint(p);
  EXPECT_EQ(diagnostics[0].message, nullptr);

  qljs_vscode_replace_text(p, /*start_line=*/0, /*start_character=*/0,
                           /*end_line=*/0, /*end_character=*/0, document_text,
                           strlen(document_text));
  // Document's text: let x;let x;
  diagnostics = qljs_vscode_lint(p);
  EXPECT_NE(diagnostics[0].message, nullptr);
  EXPECT_EQ(diagnostics[1].message, nullptr);
  EXPECT_EQ(diagnostics[1].code, nullptr);

  EXPECT_STREQ(diagnostics[0].message, "redeclaration of variable: x");
  EXPECT_STREQ(diagnostics[0].code, "E034");
  EXPECT_EQ(diagnostics[0].start_line, 0);
  EXPECT_EQ(diagnostics[0].start_character, strlen(u8"let x;let "));
  EXPECT_EQ(diagnostics[0].end_line, 0);
  EXPECT_EQ(diagnostics[0].end_character, strlen(u8"let x;let x"));

  qljs_vscode_destroy_document(p);
  qljs_vscode_destroy_workspace(workspace);
}

TEST(test_c_api_web_demo, lint_new_error_after_second_text_insertion) {
  qljs_web_demo_document* p = qljs_web_demo_create_document();

  const char8* document_text = u8"let x;";
  qljs_web_demo_set_text(p, document_text, strlen(document_text));
  const qljs_web_demo_diagnostic* diagnostics = qljs_web_demo_lint(p);
  EXPECT_EQ(diagnostics[0].message, nullptr);

  const char8* document_text_2 = u8"let x;let x;";
  qljs_web_demo_set_text(p, document_text_2, strlen(document_text_2));
  diagnostics = qljs_web_demo_lint(p);
  EXPECT_NE(diagnostics[0].message, nullptr);
  EXPECT_EQ(diagnostics[1].message, nullptr);
  EXPECT_EQ(diagnostics[1].code, nullptr);

  EXPECT_STREQ(diagnostics[0].message, "redeclaration of variable: x");
  EXPECT_STREQ(diagnostics[0].code, "E034");
  EXPECT_EQ(diagnostics[0].begin_offset, strlen(u8"let x;let "));
  EXPECT_EQ(diagnostics[0].end_offset, strlen(u8"let x;let x"));

  qljs_web_demo_destroy_document(p);
}

TEST_F(test_c_api_vscode, diagnostic_severity) {
  std::string project_dir = this->make_temporary_directory();
  std::string js_file = project_dir + "/test.js";

  qljs_vscode_workspace* workspace = qljs_vscode_create_workspace();
  qljs_vscode_document* p =
      qljs_vscode_create_source_document(workspace, js_file.c_str());

  const char8* document_text = u8"let x;let x;\nundeclaredVariable;";
  qljs_vscode_replace_text(p, /*start_line=*/0, /*start_character=*/0,
                           /*end_line=*/1, /*end_character=*/0, document_text,
                           strlen(document_text));
  const qljs_vscode_diagnostic* diagnostics = qljs_vscode_lint(p);
  EXPECT_NE(diagnostics[0].message, nullptr);
  EXPECT_NE(diagnostics[0].code, nullptr);
  EXPECT_NE(diagnostics[1].message, nullptr);
  EXPECT_NE(diagnostics[1].code, nullptr);
  EXPECT_EQ(diagnostics[2].message, nullptr);
  EXPECT_EQ(diagnostics[2].code, nullptr);

  EXPECT_EQ(diagnostics[0].severity, qljs_severity_error);
  EXPECT_EQ(diagnostics[1].severity, qljs_severity_warning);

  qljs_vscode_destroy_document(p);
  qljs_vscode_destroy_workspace(workspace);
}

TEST_F(test_c_api_vscode,
       destroying_workspace_makes_destroying_documents_unnecessary) {
  std::string project_dir = this->make_temporary_directory();
  std::string js_file_1 = project_dir + "/test-1.js";
  std::string js_file_2 = project_dir + "/test-2.js";

  qljs_vscode_workspace* workspace = qljs_vscode_create_workspace();
  [[maybe_unused]] qljs_vscode_document* doc1 =
      qljs_vscode_create_source_document(workspace, js_file_1.c_str());
  [[maybe_unused]] qljs_vscode_document* doc2 =
      qljs_vscode_create_source_document(workspace, js_file_2.c_str());
  qljs_vscode_destroy_workspace(workspace);
  // Leak checkers such as AddressSantizers would report a leak and fail the
  // test.
}

TEST_F(test_c_api_vscode, loads_config_file) {
  std::string project_dir = this->make_temporary_directory();
  std::string js_file = project_dir + "/test.js";
  std::string config_file = project_dir + "/quick-lint-js.config";
  write_file(config_file, u8R"({"globals": {"testGlobalVariable": true}})");

  qljs_vscode_workspace* workspace = qljs_vscode_create_workspace();
  qljs_vscode_document* p =
      qljs_vscode_create_source_document(workspace, js_file.c_str());

  const char8* document_text = u8"testGlobalVariable;";
  qljs_vscode_replace_text(p, /*start_line=*/0, /*start_character=*/0,
                           /*end_line=*/1, /*end_character=*/0, document_text,
                           strlen(document_text));
  const qljs_vscode_diagnostic* diagnostics = qljs_vscode_lint(p);
  EXPECT_EQ(diagnostics[0].message, nullptr);
  EXPECT_EQ(diagnostics[0].code, nullptr);

  qljs_vscode_destroy_document(p);
  qljs_vscode_destroy_workspace(workspace);
}
}
}

// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
