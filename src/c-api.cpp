// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <algorithm>
#include <cstddef>
#include <memory>
#include <quick-lint-js/assert.h>
#include <quick-lint-js/c-api-error-reporter.h>
#include <quick-lint-js/c-api.h>
#include <quick-lint-js/char8.h>
#include <quick-lint-js/configuration-loader.h>
#include <quick-lint-js/configuration.h>
#include <quick-lint-js/document.h>
#include <quick-lint-js/error.h>
#include <quick-lint-js/lint.h>
#include <quick-lint-js/lsp-location.h>
#include <quick-lint-js/padded-string.h>
#include <quick-lint-js/parse.h>
#include <quick-lint-js/web-demo-location.h>
#include <vector>

namespace quick_lint_js {
namespace {
template <class Locator, class ErrorReporter>
class qljs_document_base {
 public:
  const auto* lint() {
    this->error_reporter_.reset();
    this->error_reporter_.set_input(this->document_.string(),
                                    &this->document_.locator());
    parser p(this->document_.string(), &this->error_reporter_);
    linter l(&this->error_reporter_, &this->config_->globals());
    // TODO(strager): Use parse_and_visit_module_catching_unimplemented instead
    // of parse_and_visit_module to avoid crashing on QLJS_PARSER_UNIMPLEMENTED.
    p.parse_and_visit_module(l);

    return this->error_reporter_.get_diagnostics();
  }

  static quick_lint_js::configuration* get_default_config_file() {
    static quick_lint_js::configuration default_config;
    return &default_config;
  }

  quick_lint_js::document<Locator> document_;
  ErrorReporter error_reporter_;
  configuration* config_ = get_default_config_file();
};
}
}

struct qljs_vscode_workspace {
 public:
  qljs_vscode_document* create_source_document(const char* file_path);

  void destroy_document(qljs_vscode_document*);

 private:
  std::vector<std::unique_ptr<qljs_vscode_document>> documents_;
  quick_lint_js::basic_configuration_filesystem fs_;
  quick_lint_js::configuration_loader config_loader_ =
      quick_lint_js::configuration_loader(&this->fs_);
};

qljs_vscode_workspace* qljs_vscode_create_workspace() {
  qljs_vscode_workspace* workspace = new qljs_vscode_workspace();
  return workspace;
}

void qljs_vscode_destroy_workspace(qljs_vscode_workspace* workspace) {
  delete workspace;
}

struct qljs_vscode_document final
    : public quick_lint_js::qljs_document_base<
          quick_lint_js::lsp_locator,
          quick_lint_js::c_api_error_reporter<qljs_vscode_diagnostic,
                                              quick_lint_js::lsp_locator>> {
 public:
  explicit qljs_vscode_document(
      qljs_vscode_workspace* workspace,
      quick_lint_js::configuration_loader* config_loader, const char* file_path)
      : workspace(workspace) {
    auto config_file =
        config_loader->watch_and_load_for_file(file_path, /*token=*/this);
    if (config_file.ok()) {
      if (*config_file) {
        this->config_ = &(*config_file)->config;
      }
    }
  }

  void replace_text(int start_line, int start_character, int end_line,
                    int end_character,
                    quick_lint_js::string8_view replacement) {
    using namespace quick_lint_js;

    this->document_.replace_text(
        lsp_range{
            .start = {.line = start_line, .character = start_character},
            .end = {.line = end_line, .character = end_character},
        },
        replacement);
  }

  qljs_vscode_workspace* workspace;
};

qljs_vscode_document* qljs_vscode_workspace::create_source_document(
    const char* file_path) {
  std::unique_ptr<qljs_vscode_document>& doc =
      this->documents_.emplace_back(std::make_unique<qljs_vscode_document>(
          this, &this->config_loader_, file_path));
  return doc.get();
}

void qljs_vscode_workspace::destroy_document(qljs_vscode_document* doc) {
  auto doc_it = std::find_if(
      this->documents_.begin(), this->documents_.end(),
      [&](const std::unique_ptr<qljs_vscode_document>& existing_doc) {
        return existing_doc.get() == doc;
      });
  QLJS_ASSERT(doc_it != this->documents_.end());
  this->documents_.erase(doc_it);
}

qljs_vscode_document* qljs_vscode_create_source_document(
    qljs_vscode_workspace* workspace, const char* file_path) {
  return workspace->create_source_document(file_path);
}

void qljs_vscode_destroy_document(qljs_vscode_document* p) {
  p->workspace->destroy_document(p);
}

void qljs_vscode_replace_text(qljs_vscode_document* p, int start_line,
                              int start_character, int end_line,
                              int end_character,
                              const void* replacement_text_utf_8,
                              size_t replacement_text_byte_count) {
  p->replace_text(
      start_line, start_character, end_line, end_character,
      quick_lint_js::string8_view(
          reinterpret_cast<const quick_lint_js::char8*>(replacement_text_utf_8),
          replacement_text_byte_count));
}

const qljs_vscode_diagnostic* qljs_vscode_lint(qljs_vscode_document* p) {
  return p->lint();
}

struct qljs_web_demo_document final
    : public quick_lint_js::qljs_document_base<
          quick_lint_js::web_demo_locator,
          quick_lint_js::c_api_error_reporter<
              qljs_web_demo_diagnostic, quick_lint_js::web_demo_locator>> {
 public:
  void set_text(quick_lint_js::string8_view replacement) {
    this->document_.set_text(replacement);
  }
};

qljs_web_demo_document* qljs_web_demo_create_document(void) {
  qljs_web_demo_document* p = new qljs_web_demo_document();
  return p;
}

void qljs_web_demo_destroy_document(qljs_web_demo_document* p) { delete p; }

void qljs_web_demo_set_text(qljs_web_demo_document* p, const void* text_utf_8,
                            size_t text_byte_count) {
  p->set_text(quick_lint_js::string8_view(
      reinterpret_cast<const quick_lint_js::char8*>(text_utf_8),
      text_byte_count));
}

const qljs_web_demo_diagnostic* qljs_web_demo_lint(qljs_web_demo_document* p) {
  return p->lint();
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
