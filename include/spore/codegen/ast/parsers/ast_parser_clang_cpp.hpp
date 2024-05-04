#pragma once

#include <algorithm>
#include <functional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/HeaderSearchOptions.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Serialization/InMemoryModuleCache.h"

#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/misc/defer.hpp"

namespace spore::codegen
{
    namespace detail
    {
        struct clang_header_search : clang::HeaderSearch
        {
        };

        struct clang_diagnostics_consumer
            : clang::DiagnosticConsumer
        {
            void HandleDiagnostic(clang::DiagnosticsEngine::Level, const clang::Diagnostic&) final
            {
                SPDLOG_DEBUG("?");
            }
        };

        struct clang_codegen_consumer
            : clang::ASTConsumer,
              clang::RecursiveASTVisitor<clang_codegen_consumer>
        {
            bool traverse(clang::Decl* decl)
            {
                return clang::RecursiveASTVisitor<clang_codegen_consumer>::TraverseDecl(decl);
            }

            void HandleTranslationUnit(clang::ASTContext& context) override
            {
                clang::TranslationUnitDecl* decl = context.getTranslationUnitDecl();
                traverse(decl);
            }
        };

        struct clang_codegen_action : clang::ASTFrontendAction
        {
            std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, clang::StringRef file) final
            {
                return std::make_unique<clang_codegen_consumer>();
            }
        };
    }

    struct clang_parser_clang_cpp : ast_parser
    {
        // std::unique_ptr<detail::clang_diagnostics_consumer> diagnostics_consumer;
        // std::unique_ptr<clang::DiagnosticIDs> diagnostics_ids;
        // std::unique_ptr<clang::DiagnosticOptions> diagnostics_options;
        // std::unique_ptr<clang::DiagnosticsEngine> diagnostics;
        std::shared_ptr<clang::PCHContainerOperations> pch_container_ops;
        std::unique_ptr<clang::InMemoryModuleCache> module_cache;
        std::unique_ptr<clang::CompilerInstance> compiler;
        // std::unique_ptr<clang::FileManager> file_manager;
        // std::unique_ptr<clang::SourceManager> source_manager;
        // std::unique_ptr<clang::IdentifierTable> identifier_table;
        // std::unique_ptr<clang::SelectorTable> selector_table;
        // std::unique_ptr<clang::Builtin::Context> builtin_context;
        // std::unique_ptr<clang::ASTContext> ast_context;
        // std::shared_ptr<clang::CompilerInvocation> invocation;
        // std::shared_ptr<clang::HeaderSearchOptions> header_search_options;
        // std::unique_ptr<clang::HeaderSearch> header_search;
        // std::shared_ptr<clang::PreprocessorOptions> preprocessor_options;
        // std::shared_ptr<clang::Preprocessor> preprocessor;
        // std::unique_ptr<clang::TargetInfo> target_info;

        explicit clang_parser_clang_cpp(const codegen_options& options)
            : // diagnostics_consumer(std::make_unique<detail::clang_diagnostics_consumer>()),
              // diagnostics_ids(std::make_unique<clang::DiagnosticIDs>()),
              // diagnostics_options(std::make_unique<clang::DiagnosticOptions>()),
              // diagnostics(std::make_unique<clang::DiagnosticsEngine>(
              //     diagnostics_ids.get(), diagnostics_options.get(), diagnostics_consumer.get())),
              pch_container_ops(std::make_shared<clang::PCHContainerOperations>()),
              module_cache(std::make_unique<clang::InMemoryModuleCache>()),
              compiler(std::make_unique<clang::CompilerInstance>(pch_container_ops, module_cache.get()))
        // file_manager(std::make_unique<clang::FileManager>(clang::FileSystemOptions())),
        // source_manager(std::make_unique<clang::SourceManager>(*diagnostics, *file_manager)),
        // identifier_table(std::make_unique<clang::IdentifierTable>()),
        // selector_table(std::make_unique<clang::SelectorTable>()),
        // builtin_context(std::make_unique<clang::Builtin::Context>()),
        // invocation(std::make_shared<clang::CompilerInvocation>())
        // header_search_options(std::make_shared<clang::HeaderSearchOptions>()),
        // preprocessor_options(std::make_shared<clang::PreprocessorOptions>())
        {
            std::vector<std::string> args;
            //args.emplace_back("-xc++");
            args.emplace_back("-w");
            args.emplace_back(fmt::format("-std={}", options.cpp_standard));

            for (const std::string& include : options.includes)
            {
                args.emplace_back(fmt::format("-I{}", include));
            }

            for (const auto& [key, value] : options.definitions)
            {
                if (value.empty())
                {
                    args.emplace_back(fmt::format("-D{}", key));
                }
                else
                {
                    args.emplace_back(fmt::format("-D{}={}", key, value));
                }
            }

            std::sort(args.begin(), args.end());
            args.erase(std::unique(args.begin(), args.end()), args.end());

            std::vector<const char*> args_view;
            args_view.resize(args.size());

            std::transform(args.begin(), args.end(), args_view.begin(),
                [](const std::string& value) { return value.data(); });

            // diagnostics->setSuppressAllDiagnostics(true);
            // diagnostics->setClient(diagnostics_consumer.get(), false);
            // compiler->setDiagnostics(diagnostics.get());
            // compiler->setFileManager(file_manager.get());
            // compiler->setSourceManager(source_manager.get());

            // ast_context = std::make_unique<clang::ASTContext>(
            //    compiler->getLangOpts(),
            //    *source_manager,
            //    *identifier_table,
            //    *selector_table,
            //    *builtin_context,
            //    clang::TranslationUnitKind::TU_Complete);

            // clang::DiagnosticOptions diagnostic_options;

//            clang::LangOptions& lang_options = *compiler->getInvocation().LangOpts;
//            lang_options.LangStd = clang::LangStandard::lang_cxx20;
//
//            clang::TargetOptions& target_options = *compiler->getInvocation().TargetOpts;
//
//            clang::HeaderSearchOptions& header_search_opts = *compiler->getInvocation().HeaderSearchOpts;
//            for (const std::string& include : options.includes)
//            {
//                constexpr bool is_framework = false;
//                constexpr bool ignore_sys_root = false;
//                header_search_opts.AddPath(include, clang::frontend::IncludeDirGroup::Angled, is_framework, ignore_sys_root);
//            }
//
//            clang::PreprocessorOptions& preprocessor_opts = *compiler->getInvocation().PreprocessorOpts;
//            for (const auto& [key, value] : options.definitions)
//            {
//                if (value.empty())
//                {
//                    preprocessor_opts.addMacroDef(fmt::format("{}", key));
//                }
//                else
//                {
//                    preprocessor_opts.addMacroDef(fmt::format("{}={}", key, value));
//                }
//            }

            // compiler->setASTContext(ast_context.get());
            compiler->createDiagnostics(new detail::clang_diagnostics_consumer());
            // compiler->setDiagnostics(
            //     clang::CompilerInstance::createDiagnostics(
            //         new clang::DiagnosticOptions(std::move(diagnostic_options)),
            //         diagnostics_consumer.get()));

            clang::CompilerInvocation::CreateFromArgs(compiler->getInvocation(), args_view, compiler->getDiagnostics());
            // compiler->setInvocation(invocation);

            compiler->createFileManager();
            compiler->createSourceManager(compiler->getFileManager());
            bool success = compiler->createTarget();

            compiler->createPreprocessor(clang::TranslationUnitKind::TU_Complete);
            compiler->createASTContext();

            compiler->LoadRequestedPlugins();

            //            header_search = std::make_unique<clang::HeaderSearch>(
            //                header_search_options,
            //                *source_manager,
            //                *diagnostics,
            //                compiler->getLangOpts(),
            //                &compiler->getTarget());
            //
            //            preprocessor = std::make_shared<clang::Preprocessor>(
            //                preprocessor_options,
            //                *diagnostics,
            //                compiler->getLangOpts(),
            //                *source_manager,
            //                *header_search,
            //                *compiler);
            //
            //            compiler->setPreprocessor(preprocessor);
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            clang::InputKind input_kind {clang::Language::CXX};
            clang::FrontendInputFile input_file {path, input_kind};

            detail::clang_codegen_action action;
            action.setCurrentInput(input_file);

            return compiler->ExecuteAction(action);
        }
    };
}

#if 0

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/ASTConsumers.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

//===----------------------------------------------------------------------===//
/// ASTPrinter - Pretty-printer and dumper of ASTs

namespace {
  class ASTPrinter : public ASTConsumer,
                     public RecursiveASTVisitor<ASTPrinter> {
    typedef RecursiveASTVisitor<ASTPrinter> base;

  public:
    enum Kind { DumpFull, Dump, Print, None };
    ASTPrinter(std::unique_ptr<raw_ostream> Out, Kind K,
               ASTDumpOutputFormat Format, StringRef FilterString,
               bool DumpLookups = false, bool DumpDeclTypes = false)
        : Out(Out ? *Out : llvm::outs()), OwnedOut(std::move(Out)),
          OutputKind(K), OutputFormat(Format), FilterString(FilterString),
          DumpLookups(DumpLookups), DumpDeclTypes(DumpDeclTypes) {}

    void HandleTranslationUnit(ASTContext &Context) override {
      TranslationUnitDecl *D = Context.getTranslationUnitDecl();

      if (FilterString.empty())
        return print(D);

      TraverseDecl(D);
    }

    bool shouldWalkTypesOfTypeLocs() const { return false; }

    bool TraverseDecl(Decl *D) {
      if (D && filterMatches(D)) {
        bool ShowColors = Out.has_colors();
        if (ShowColors)
          Out.changeColor(raw_ostream::BLUE);

        if (OutputFormat == ADOF_Default)
          Out << (OutputKind != Print ? "Dumping " : "Printing ") << getName(D)
              << ":\n";

        if (ShowColors)
          Out.resetColor();
        print(D);
        Out << "\n";
        // Don't traverse child nodes to avoid output duplication.
        return true;
      }
      return base::TraverseDecl(D);
    }

  private:
    std::string getName(Decl *D) {
      if (isa<NamedDecl>(D))
        return cast<NamedDecl>(D)->getQualifiedNameAsString();
      return "";
    }
    bool filterMatches(Decl *D) {
      return getName(D).find(FilterString) != std::string::npos;
    }
    void print(Decl *D) {
      if (DumpLookups) {
        if (DeclContext *DC = dyn_cast<DeclContext>(D)) {
          if (DC == DC->getPrimaryContext())
            DC->dumpLookups(Out, OutputKind != None, OutputKind == DumpFull);
          else
            Out << "Lookup map is in primary DeclContext "
                << DC->getPrimaryContext() << "\n";
        } else
          Out << "Not a DeclContext\n";
      } else if (OutputKind == Print) {
        PrintingPolicy Policy(D->getASTContext().getLangOpts());
        D->print(Out, Policy, /*Indentation=*/0, /*PrintInstantiation=*/true);
      } else if (OutputKind != None) {
        D->dump(Out, OutputKind == DumpFull, OutputFormat);
      }

      if (DumpDeclTypes) {
        Decl *InnerD = D;
        if (auto *TD = dyn_cast<TemplateDecl>(D))
          InnerD = TD->getTemplatedDecl();

        // FIXME: Support OutputFormat in type dumping.
        // FIXME: Support combining -ast-dump-decl-types with -ast-dump-lookups.
        if (auto *VD = dyn_cast<ValueDecl>(InnerD))
          VD->getType().dump(Out, VD->getASTContext());
        if (auto *TD = dyn_cast<TypeDecl>(InnerD))
          TD->getTypeForDecl()->dump(Out, TD->getASTContext());
      }
    }

    raw_ostream &Out;
    std::unique_ptr<raw_ostream> OwnedOut;

    /// How to output individual declarations.
    Kind OutputKind;

    /// What format should the output take?
    ASTDumpOutputFormat OutputFormat;

    /// Which declarations or DeclContexts to display.
    std::string FilterString;

    /// Whether the primary output is lookup results or declarations. Individual
    /// results will be output with a format determined by OutputKind. This is
    /// incompatible with OutputKind == Print.
    bool DumpLookups;

    /// Whether to dump the type for each declaration dumped.
    bool DumpDeclTypes;
  };

  class ASTDeclNodeLister : public ASTConsumer,
                     public RecursiveASTVisitor<ASTDeclNodeLister> {
  public:
    ASTDeclNodeLister(raw_ostream *Out = nullptr)
        : Out(Out ? *Out : llvm::outs()) {}

    void HandleTranslationUnit(ASTContext &Context) override {
      TraverseDecl(Context.getTranslationUnitDecl());
    }

    bool shouldWalkTypesOfTypeLocs() const { return false; }

    bool VisitNamedDecl(NamedDecl *D) {
      D->printQualifiedName(Out);
      Out << '\n';
      return true;
    }

  private:
    raw_ostream &Out;
  };
} // end anonymous namespace
#endif