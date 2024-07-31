#pragma once

#include <algorithm>
#include <functional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/misc/defer.hpp"

namespace spore::codegen
{
    namespace detail
    {
        struct ASTVisitorImpl : public clang::RecursiveASTVisitor<ASTVisitorImpl>
        {
            clang::ASTContext& Context;
            std::string File;

            explicit ASTVisitorImpl(clang::ASTContext& Context, llvm::StringRef InFile)
                : Context(Context),
                  File(InFile.data()) {}

            bool shouldWalkTypesOfTypeLocs() const
            {
                return false;
            }
            bool shouldVisitLambdaBody() const
            {
                return false;
            }

            //            class ASTContext;
            //            class ClassTemplateDecl;
            //            class ConstructorUsingShadowDecl;
            //            class CXXBasePath;
            //            class CXXBasePaths;
            //            class CXXConstructorDecl;
            //            class CXXDestructorDecl;
            //            class CXXFinalOverriderMap;
            //            class CXXIndirectPrimaryBaseSet;
            //            class CXXMethodDecl;
            //            class DecompositionDecl;
            //            class FriendDecl;
            //            class FunctionTemplateDecl;
            //            class IdentifierInfo;
            //            class MemberSpecializationInfo;
            //            class BaseUsingDecl;
            //            class TemplateDecl;
            //            class TemplateParameterList;
            //            class UsingDecl;

            bool VisitCXXMethodDecl(clang::CXXMethodDecl* Declaration)
            {
                return true;
            }

            bool VisitFunctionTemplateDecl(clang::FunctionTemplateDecl* Declaration)
            {
                return true;
            }

            bool VisitTemplateDecl(clang::TemplateDecl* Declaration)
            {
                return true;
            }

            bool VisitClassTemplateDecl(clang::ClassTemplateDecl* Declaration)
            {
                return true;
            }

            bool VisitCXXRecordDecl(clang::CXXRecordDecl* Declaration)
            {
                // clang::FullSourceLoc FullLocation = Context.getFullLoc(Declaration->getBeginLoc());
                // if (File != FullLocation.getFileEntry()->getName())
                //     return true;
                if (!Declaration)
                {
                    return true;
                }

                clang::FullSourceLoc FullLocation = Context.getFullLoc(Declaration->getBeginLoc());
                SPDLOG_INFO("declaration {}", Declaration->getName().data());
                return true;
            }
        };

        struct ASTConsumerImpl : public clang::ASTConsumer
        {
            ASTVisitorImpl Visitor;
            std::string File;

            explicit ASTConsumerImpl(clang::ASTContext& Context, llvm::StringRef InFile)
                : Visitor(Context, InFile),
                  File(InFile.data()) {}

            void HandleTranslationUnit(clang::ASTContext& Context) override
            {
                // Visitor.TraverseDecl(Context.getTranslationUnitDecl());

                for (clang::Decl* decl : Context.getTranslationUnitDecl()->decls())
                {
                    const auto& file_id = Context.getSourceManager().getFileID(decl->getLocation());
                    if (file_id == Context.getSourceManager().getMainFileID())
                    {
                        Visitor.TraverseDecl(decl);
                    }
                }
            }
        };

        struct ASTFrontendActionImpl : public clang::ASTFrontendAction
        {
            std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& Compiler, llvm::StringRef InFile) override
            {
                return std::make_unique<ASTConsumerImpl>(Compiler.getASTContext(), InFile);
            }
        };

        struct FrontendActionFactoryImpl : public clang::tooling::FrontendActionFactory
        {
            std::unique_ptr<clang::FrontendAction> create() override
            {
                return std::make_unique<ASTFrontendActionImpl>();
            }
        };
    }

    struct ast_parser_clang_cpp : ast_parser
    {
        std::vector<std::string> args;
        std::vector<const char*> argv;
        std::shared_ptr<clang::PCHContainerOperations> pch_container_ops;

        explicit ast_parser_clang_cpp(const codegen_options& options)
            : pch_container_ops(std::make_shared<clang::PCHContainerOperations>())
        {
            args.emplace_back("-xc++");
            args.emplace_back("-fsyntax-only");
            args.emplace_back("-E");
            args.emplace_back("-dM");
            args.emplace_back("-w");
            args.emplace_back("-Wno-everything");
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

            const auto transformer = [](const std::string& arg) { return arg.data(); };
            std::transform(args.begin(), args.end(), std::back_inserter(argv), transformer);
        }

        bool parse_files(std::span<const std::filesystem::path> paths, std::vector<ast_file>& files)
        {
            std::vector<std::string> action_args = args;
            for (const std::filesystem::path& path : paths)
            {
                action_args.emplace_back(path.generic_string());
            }

            std::vector<const char*> action_argv;
            action_argv.emplace_back(SPORE_CODEGEN_NAME);
            const auto transformer = [](const std::string& arg) { return arg.data(); };
            std::transform(action_args.begin(), action_args.end(), std::back_inserter(action_argv), transformer);
            int action_argc = static_cast<int>(action_argv.size());

            llvm::cl::OptionCategory tool_category {SPORE_CODEGEN_NAME};
            llvm::Expected<clang::tooling::CommonOptionsParser> expected_options_parser =
                clang::tooling::CommonOptionsParser::create(action_argc, action_argv.data(), tool_category);

            if (!expected_options_parser)
            {
                llvm::errs() << expected_options_parser.takeError();
                SPDLOG_ERROR("cannot create option parser");
                return false;
            }

            clang::tooling::CommonOptionsParser& options_parser = expected_options_parser.get();
            clang::tooling::ClangTool tool(options_parser.getCompilations(), options_parser.getSourcePathList());

            detail::FrontendActionFactoryImpl factory;
            return tool.run(&factory);
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            SPDLOG_INFO("parsing, path={}", path);

            std::string source;
            if (!read_file(path, source))
            {
                SPDLOG_ERROR("cannot read source file, path={}", path);
                return false;
            }

            //            int argc = static_cast<int>(argv.size());
            //            llvm::cl::OptionCategory tool_category {SPORE_CODEGEN_NAME};
            //            llvm::Expected<clang::tooling::CommonOptionsParser> expected_options_parser =
            //                clang::tooling::CommonOptionsParser::create(argc, argv.data(), tool_category);
            //
            //            if (!expected_options_parser)
            //            {
            //                SPDLOG_ERROR("cannot create option parser");
            //                return false;
            //            }
            //
            //            clang::tooling::CommonOptionsParser& options_parser = expected_options_parser.get();
            //            clang::tooling::ClangTool tool(options_parser.getCompilations(), options_parser.getSourcePathList());
            //
            //            return tool.run(std::make_unique<detail::FindNamedClassAction>());
            return clang::tooling::runToolOnCodeWithArgs(
                std::make_unique<detail::ASTFrontendActionImpl>(), source, args, path, SPORE_CODEGEN_NAME, pch_container_ops);
        }

        bool parse_files(const std::vector<std::string>& paths, std::vector<ast_file>& files) override
        {
            return false;
        }
    };
}

#if 0

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
            void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& diagnostic) final
            {
                (void) level;
                (void) diagnostic;
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

            bool isModelParsingAction() const final
            {
                return true;
            }
        };
    }

    struct ast_parser_clang_cpp : ast_parser
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

        explicit ast_parser_clang_cpp(const codegen_options& options)
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
            // args.emplace_back("-xc++");
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

            clang::FrontendOptions& frontend_options = compiler->getInvocation().getFrontendOpts();
            frontend_options.SkipFunctionBodies = true;
            frontend_options.ProgramAction = clang::frontend::ParseSyntaxOnly;
            // compiler->setInvocation(invocation);

            compiler->createFileManager();
            compiler->createSourceManager(compiler->getFileManager());
            bool success = compiler->createTarget();

            compiler->createPreprocessor(clang::TranslationUnitKind::TU_Complete);
            compiler->createASTContext();

            // compiler->LoadRequestedPlugins();

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

            compiler->ExecuteAction(action);
            return compiler->ExecuteAction(action);
        }

      private:
        static inline const defer _llvm_shutdown_defer = &llvm::llvm_shutdown;
    };
}
#endif
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