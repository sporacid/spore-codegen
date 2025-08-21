#include "spore/codegen/parsers/cpp/codegen_parser_cpp.hpp"

#include <format>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_macros.hpp"
#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/parsers/cpp/codegen_utils_cpp.hpp"
#include "spore/codegen/utils/strings.hpp"

SPORE_CODEGEN_PUSH_DISABLE_WARNINGS
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Config/llvm-config.h"
SPORE_CODEGEN_POP_DISABLE_WARNINGS

namespace spore::codegen
{
    namespace detail
    {
        clang::PrintingPolicy get_printing_policy(const clang::ASTContext& ast_context)
        {
            clang::PrintingPolicy policy {ast_context.getLangOpts()};
            policy.SuppressTagKeyword = true;
            policy.ConstantsAsWritten = true;
            policy.Bool = true;
            return policy;
        }

        std::string get_spelling(const clang::ASTContext& ast_context, const clang::SourceRange source_range)
        {
            const clang::SourceManager& source_manager = ast_context.getSourceManager();
            const clang::LangOptions& lang_opts = ast_context.getLangOpts();
            const clang::CharSourceRange char_range = clang::CharSourceRange::getTokenRange(source_range);
            return clang::Lexer::getSourceText(char_range, source_manager, lang_opts).str();
        }

        std::string make_templated_name(const clang::ASTContext& ast_context, const clang::NamedDecl& decl)
        {
            std::string templated_name;
            llvm::raw_string_ostream stream {templated_name};

            decl.printName(stream, get_printing_policy(ast_context));

            if (const clang::TemplateParameterList* template_params = decl.getDescribedTemplateParams())
            {
                templated_name += "<";

                for (std::size_t index = 0; index < template_params->size(); ++index)
                {
                    const clang::NamedDecl* template_param = template_params->getParam(index);

                    if (index > 0)
                    {
                        templated_name += ", ";
                    }

                    templated_name += template_param->getNameAsString();

                    if (template_param->isParameterPack())
                    {
                        templated_name += "...";
                    }
                }

                templated_name += ">";
            }

            return templated_name;
        }

        std::string make_scope(const clang::ASTContext& ast_context, const clang::Decl& decl)
        {
            constexpr std::string_view separator = "::";

            std::string scope;

            const clang::DeclContext* context = decl.getDeclContext();

            while (context != nullptr && not context->isTranslationUnit())
            {
                if (const clang::NamedDecl* named_decl = llvm::dyn_cast<clang::NamedDecl>(context))
                {
                    scope.insert(0, separator);
                    scope.insert(0, make_templated_name(ast_context, *named_decl));
                }

                context = context->getParent();
            }

            if (scope.ends_with(separator))
            {
                scope.resize(scope.size() - separator.size());
            }

            return scope;
        }

        std::vector<std::string> make_template_specialization_params(const clang::ASTContext& ast_context, const clang::ASTTemplateArgumentListInfo& template_args)
        {
            const auto transformer = [&](const clang::TemplateArgumentLoc& template_arg) {
                return get_spelling(ast_context, template_arg.getSourceRange());
            };

            std::vector<std::string> template_specialization_params;
            template_specialization_params.reserve(template_args.NumTemplateArgs);

            std::ranges::transform(template_args.arguments(), std::back_inserter(template_specialization_params), transformer);

            return template_specialization_params;
        }

        std::vector<cpp_template_param> make_template_params(clang::ASTContext& ast_context, const clang::TemplateParameterList& template_params)
        {
            std::vector<cpp_template_param> cpp_template_params;

            std::size_t index = 0;

            for (const clang::NamedDecl* template_param_decl : template_params)
            {
                cpp_template_param& cpp_template_param = cpp_template_params.emplace_back();
                cpp_template_param.name = template_param_decl->getNameAsString();
                cpp_template_param.is_variadic = template_param_decl->isTemplateParameterPack();

                if (cpp_template_param.name.empty())
                {
                    cpp_template_param.name = std::format("_t{}", index);
                    ++index;
                }

                if (const clang::TemplateTypeParmDecl* type_param_decl = llvm::dyn_cast<clang::TemplateTypeParmDecl>(template_param_decl))
                {
                    cpp_template_param.kind = cpp_template_param_kind::type;

                    if (const clang::TypeConstraint* type_constraint = type_param_decl->getTypeConstraint())
                    {
                        llvm::raw_string_ostream stream {cpp_template_param.type};
                        type_constraint->print(stream, get_printing_policy(ast_context));
                    }
                    else if (type_param_decl->wasDeclaredWithTypename())
                    {
                        cpp_template_param.type = "typename";
                    }
                    else
                    {
                        cpp_template_param.type = "class";
                    }

                    if (type_param_decl->hasDefaultArgument())
                    {
#if LLVM_VERSION_MAJOR >= 19
                        cpp_template_param.default_value = type_param_decl->getDefaultArgument().getArgument().getAsType().getAsString(get_printing_policy(ast_context));
#else
                        cpp_template_param.default_value = type_param_decl->getDefaultArgument().getAsString(get_printing_policy(ast_context));
#endif
                    }
                }
                else if (const clang::NonTypeTemplateParmDecl* non_type_param_decl = llvm::dyn_cast<clang::NonTypeTemplateParmDecl>(template_param_decl))
                {
                    cpp_template_param.kind = cpp_template_param_kind::non_type;
                    cpp_template_param.type = non_type_param_decl->getType().getAsString(get_printing_policy(ast_context));

                    if (non_type_param_decl->hasDefaultArgument())
                    {
#if LLVM_VERSION_MAJOR >= 19
                        const clang::SourceRange source_range = non_type_param_decl->getDefaultArgument().getSourceExpression()->getSourceRange();
#else
                        const clang::SourceRange source_range = non_type_param_decl->getDefaultArgument()->getSourceRange();
#endif
                        cpp_template_param.default_value = get_spelling(ast_context, source_range);
                    }
                }
                else if (const clang::TemplateTemplateParmDecl* template_template_param_decl = llvm::dyn_cast<clang::TemplateTemplateParmDecl>(template_param_decl))
                {
                    cpp_template_param.kind = cpp_template_param_kind::template_;

                    // TODO @sporacid Is there a better way?

                    llvm::raw_string_ostream stream(cpp_template_param.type);
                    template_template_param_decl->print(stream, get_printing_policy(ast_context));

                    const std::string template_param_name = template_template_param_decl->getNameAsString();
                    cpp_template_param.type.erase(cpp_template_param.type.size() - template_param_name.size());

                    strings::trim_end(cpp_template_param.type);

                    if (template_template_param_decl->hasDefaultArgument())
                    {
                        cpp_template_param.default_value = get_spelling(ast_context, template_template_param_decl->getDefaultArgument().getSourceRange());
                    }
                }

                if (cpp_template_param.is_variadic)
                {
                    cpp_template_param.type += "...";
                }
            }

            return cpp_template_params;
        }

        nlohmann::json make_attributes(const clang::Decl& decl)
        {
            nlohmann::json json;

            for (const clang::Attr* attr : decl.attrs())
            {
                if (const clang::AnnotateAttr* annotate_attr = llvm::dyn_cast<clang::AnnotateAttr>(attr))
                {
                    const std::string annotate_value = annotate_attr->getAnnotation().str();

                    if (not cpp::parse_pairs(annotate_value, json))
                    {
                        SPDLOG_WARN("invalid attributes, value={}", annotate_value);
                    }
                }
            }

            return json;
        }

        cpp_flags make_access_flags(const clang::AccessSpecifier access_spec)
        {
            switch (access_spec)
            {
                case clang::AS_public:
                    return cpp_flags::public_;
                case clang::AS_private:
                    return cpp_flags::private_;
                case clang::AS_protected:
                    return cpp_flags::protected_;
                default:
                    return cpp_flags::none;
            }
        }

        cpp_ref make_ref(clang::ASTContext& ast_context, const clang::QualType& type, const bool is_variadic = false)
        {
            cpp_ref cpp_ref;
            cpp_ref.name = type.getAsString(get_printing_policy(ast_context));
            cpp_ref.is_variadic = is_variadic;

            if (is_variadic and not cpp_ref.name.ends_with("..."))
            {
                cpp_ref.name += "...";
            }

            if (type.isConstQualified())
            {
                cpp_ref.flags = cpp_ref.flags | cpp_flags::const_;
            }

            if (type.isVolatileQualified())
            {
                cpp_ref.flags = cpp_ref.flags | cpp_flags::volatile_;
            }

            return cpp_ref;
        }

        cpp_variable make_variable(clang::ASTContext& ast_context, const clang::VarDecl& var_decl)
        {
            cpp_variable cpp_variable;
            cpp_variable.name = var_decl.getNameAsString();
            cpp_variable.scope = make_scope(ast_context, var_decl);
            cpp_variable.attributes = make_attributes(var_decl);
            cpp_variable.type = make_ref(ast_context, var_decl.getType(), var_decl.isParameterPack());

            if (var_decl.isConstexpr())
            {
                cpp_variable.flags = cpp_variable.flags | cpp_flags::constexpr_;
            }

            if (var_decl.isInlineSpecified())
            {
                cpp_variable.flags = cpp_variable.flags | cpp_flags::inline_;
            }

            if (var_decl.isStaticLocal() or (var_decl.hasGlobalStorage() and var_decl.getStorageClass() == clang::SC_Static))
            {
                cpp_variable.flags = cpp_variable.flags | cpp_flags::static_;
            }

            if (var_decl.hasInit())
            {
                if (const clang::Expr* default_value_expr = var_decl.getInit())
                {
                    cpp_variable.default_value = get_spelling(ast_context, default_value_expr->getSourceRange());
                }
            }

            if (const clang::TemplateParameterList* template_params = var_decl.getDescribedTemplateParams())
            {
                cpp_variable.template_params = make_template_params(ast_context, *template_params);
            }

            if (const clang::VarTemplateSpecializationDecl* template_spec_decl = llvm::dyn_cast<clang::VarTemplateSpecializationDecl>(&var_decl))
            {
#if LLVM_VERSION_MAJOR >= 19
                const clang::ASTTemplateArgumentListInfo* template_spec_args = template_spec_decl->getTemplateArgsAsWritten();
#else
                const clang::ASTTemplateArgumentListInfo* template_spec_args = template_spec_decl->getTemplateArgsInfo();
#endif
                cpp_variable.template_specialization_params = make_template_specialization_params(ast_context, *template_spec_args);
            }

            return cpp_variable;
        }

        cpp_argument make_argument(clang::ASTContext& ast_context, const clang::ParmVarDecl& param_decl)
        {
            cpp_argument cpp_argument;
            cpp_argument.name = param_decl.getNameAsString();
            cpp_argument.type = make_ref(ast_context, param_decl.getType(), param_decl.isParameterPack());
            cpp_argument.attributes = make_attributes(param_decl);
            cpp_argument.is_variadic = param_decl.isParameterPack();

            if (cpp_argument.name.empty())
            {
                cpp_argument.name = std::format("_arg{}", param_decl.getFunctionScopeIndex());
            }

            if (param_decl.hasDefaultArg())
            {
                cpp_argument.default_value = get_spelling(ast_context, param_decl.getDefaultArg()->getSourceRange());
            }

            return cpp_argument;
        }

        cpp_function make_function(clang::ASTContext& ast_context, const clang::FunctionDecl& function_decl)
        {
            cpp_function cpp_function;
            cpp_function.name = function_decl.getNameAsString();
            cpp_function.scope = make_scope(ast_context, function_decl);
            cpp_function.attributes = make_attributes(function_decl);
            cpp_function.return_type = make_ref(ast_context, function_decl.getReturnType());

            if (function_decl.isConstexpr())
            {
                cpp_function.flags = cpp_function.flags | cpp_flags::constexpr_;
            }

            if (function_decl.isConsteval())
            {
                cpp_function.flags = cpp_function.flags | cpp_flags::consteval_;
            }

            if (function_decl.isInlineSpecified())
            {
                cpp_function.flags = cpp_function.flags | cpp_flags::inline_;
            }

            if (function_decl.isStatic())
            {
                cpp_function.flags = cpp_function.flags | cpp_flags::static_;
            }

            if (const clang::CXXMethodDecl* method_decl = llvm::dyn_cast<clang::CXXMethodDecl>(&function_decl))
            {
                if (method_decl->isConst())
                {
                    cpp_function.flags = cpp_function.flags | cpp_flags::const_;
                }

                if (method_decl->isVirtual())
                {
                    cpp_function.flags = cpp_function.flags | cpp_flags::virtual_;
                }

                if (method_decl->isVolatile())
                {
                    cpp_function.flags = cpp_function.flags | cpp_flags::volatile_;
                }
            }

            if (const clang::TemplateParameterList* template_params = function_decl.getDescribedTemplateParams())
            {
                cpp_function.template_params = make_template_params(ast_context, *template_params);
            }

            if (const clang::FunctionTemplateSpecializationInfo* template_spec_info = function_decl.getTemplateSpecializationInfo())
            {
                cpp_function.template_specialization_params = make_template_specialization_params(ast_context, *template_spec_info->TemplateArgumentsAsWritten);
            }

            for (const clang::ParmVarDecl* param_decl : function_decl.parameters())
            {
                cpp_function.arguments.emplace_back(make_argument(ast_context, *param_decl));
            }

            return cpp_function;
        }

        cpp_constructor make_constructor(clang::ASTContext& ast_context, const clang::CXXConstructorDecl& ctor_decl)
        {
            cpp_constructor cpp_constructor;
            cpp_constructor.attributes = make_attributes(ctor_decl);

            if (ctor_decl.isConstexpr())
            {
                cpp_constructor.flags = cpp_constructor.flags | cpp_flags::constexpr_;
            }

            if (ctor_decl.isConsteval())
            {
                cpp_constructor.flags = cpp_constructor.flags | cpp_flags::consteval_;
            }

            if (ctor_decl.isInlineSpecified())
            {
                cpp_constructor.flags = cpp_constructor.flags | cpp_flags::inline_;
            }

            if (ctor_decl.isExplicit())
            {
                cpp_constructor.flags = cpp_constructor.flags | cpp_flags::explicit_;
            }

            if (ctor_decl.isDefaultConstructor())
            {
                cpp_constructor.flags = cpp_constructor.flags | cpp_flags::default_;
            }

            if (const clang::TemplateParameterList* template_params = ctor_decl.getDescribedTemplateParams())
            {
                cpp_constructor.template_params = make_template_params(ast_context, *template_params);
            }

            for (const clang::ParmVarDecl* param_decl : ctor_decl.parameters())
            {
                cpp_constructor.arguments.emplace_back(make_argument(ast_context, *param_decl));
            }

            return cpp_constructor;
        }

        cpp_enum make_enum(clang::ASTContext& ast_context, const clang::EnumDecl& enum_decl)
        {
            cpp_enum cpp_enum;
            cpp_enum.name = enum_decl.getNameAsString();
            cpp_enum.scope = make_scope(ast_context, enum_decl);
            cpp_enum.base = make_ref(ast_context, enum_decl.getIntegerType());
            cpp_enum.attributes = make_attributes(enum_decl);
            cpp_enum.definition = enum_decl.isThisDeclarationADefinition();

            if (enum_decl.isScopedUsingClassTag())
            {
                cpp_enum.type = cpp_enum_type::enum_class;
            }
            else
            {
                cpp_enum.type = cpp_enum_type::enum_;
            }

            if (cpp_enum.definition)
            {
                if (const clang::CXXRecordDecl* parent_decl = llvm::dyn_cast<clang::CXXRecordDecl>(enum_decl.getDeclContext()))
                {
                    cpp_enum.nested = true;
                    std::ignore = parent_decl;
                }

                for (const clang::EnumConstantDecl* enum_constant_decl : enum_decl.enumerators())
                {
                    cpp_enum_value& cpp_enum_value = cpp_enum.values.emplace_back();
                    cpp_enum_value.name = enum_constant_decl->getNameAsString();
                    cpp_enum_value.value = enum_constant_decl->getValue().getSExtValue();
                    cpp_enum_value.attributes = make_attributes(*enum_constant_decl);
                }
            }

            return cpp_enum;
        }

        cpp_class make_class(clang::ASTContext& ast_context, const clang::CXXRecordDecl& class_decl)
        {
            cpp_class cpp_class;
            cpp_class.name = class_decl.getNameAsString();
            cpp_class.scope = make_scope(ast_context, class_decl);
            cpp_class.attributes = make_attributes(class_decl);
            cpp_class.definition = class_decl.isThisDeclarationADefinition();

            if (class_decl.isStruct())
            {
                cpp_class.type = cpp_class_type::struct_;
            }
            else if (class_decl.isClass())
            {
                cpp_class.type = cpp_class_type::class_;
            }

            if (const clang::TemplateParameterList* template_params = class_decl.getDescribedTemplateParams())
            {
                cpp_class.template_params = make_template_params(ast_context, *template_params);
            }

            if (const clang::ClassTemplateSpecializationDecl* template_spec_decl = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(&class_decl))
            {
#if LLVM_VERSION_MAJOR >= 19
                const clang::ASTTemplateArgumentListInfo* template_spec_args = template_spec_decl->getTemplateArgsAsWritten();
#else
                const clang::ASTTemplateArgumentListInfo* template_spec_args = template_spec_decl->getTemplateArgsInfo();
#endif
                cpp_class.template_specialization_params = make_template_specialization_params(ast_context, *template_spec_args);
            }

            if (cpp_class.definition)
            {
                if (const clang::CXXRecordDecl* parent_decl = llvm::dyn_cast<clang::CXXRecordDecl>(class_decl.getDeclContext()))
                {
                    cpp_class.nested = true;
                    std::ignore = parent_decl;
                }

                for (const clang::CXXBaseSpecifier& base_specifier : class_decl.bases())
                {
                    cpp_ref cpp_ref = make_ref(ast_context, base_specifier.getType(), base_specifier.isPackExpansion());

                    cpp_ref.flags = cpp_ref.flags | make_access_flags(base_specifier.getAccessSpecifier());

                    if (base_specifier.isVirtual())
                    {
                        cpp_ref.flags = cpp_ref.flags | cpp_flags::virtual_;
                    }

                    cpp_class.bases.emplace_back(std::move(cpp_ref));
                }

                for (const clang::FieldDecl* field_decl : class_decl.fields())
                {
                    cpp_field& cpp_field = cpp_class.fields.emplace_back();
                    cpp_field.name = field_decl->getNameAsString();
                    cpp_field.type = make_ref(ast_context, field_decl->getType(), field_decl->isParameterPack());
                    cpp_field.attributes = make_attributes(*field_decl);
                    cpp_field.flags = cpp_field.flags | make_access_flags(field_decl->getAccess());

                    if (field_decl->isMutable())
                    {
                        cpp_field.flags = cpp_field.flags | cpp_flags::mutable_;
                    }

                    if (field_decl->hasInClassInitializer())
                    {
                        if (const clang::Expr* default_value_expr = field_decl->getInClassInitializer())
                        {
                            cpp_field.default_value = get_spelling(ast_context, default_value_expr->getSourceRange());
                        }
                    }
                }

                for (const clang::Decl* decl : class_decl.decls())
                {
                    std::vector<cpp_template_param> template_params;

                    if (const clang::FunctionTemplateDecl* function_template_decl = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
                    {
                        template_params = make_template_params(ast_context, *function_template_decl->getTemplateParameters());
                        decl = function_template_decl->getTemplatedDecl();
                    }

                    if (const clang::CXXConstructorDecl* ctor_decl = llvm::dyn_cast<clang::CXXConstructorDecl>(decl))
                    {
                        if (not ctor_decl->isImplicit())
                        {
                            cpp_constructor cpp_constructor = make_constructor(ast_context, *ctor_decl);
                            cpp_constructor.template_params = std::move(template_params);
                            cpp_constructor.flags = cpp_constructor.flags | make_access_flags(ctor_decl->getAccess());
                            cpp_class.constructors.emplace_back(std::move(cpp_constructor));
                        }
                    }
                    else if (const clang::FunctionDecl* function_decl = llvm::dyn_cast<clang::FunctionDecl>(decl))
                    {
                        if (not function_decl->isImplicit())
                        {
                            cpp_function cpp_function = make_function(ast_context, *function_decl);
                            cpp_function.template_params = std::move(template_params);
                            cpp_function.flags = cpp_function.flags | make_access_flags(function_decl->getAccess());
                            cpp_class.functions.emplace_back(std::move(cpp_function));
                        }
                    }
                }
            }

            return cpp_class;
        }

        struct frontend_action_context
        {
            std::vector<cpp_file>& cpp_files;
            std::map<std::string_view, std::size_t>& cpp_file_map;
        };

        struct ast_visitor : clang::RecursiveASTVisitor<ast_visitor>
        {
            frontend_action_context& action_context;
            clang::ASTContext& ast_context;

            explicit ast_visitor(frontend_action_context& action_context, clang::ASTContext& ast_context)
                : action_context(action_context),
                  ast_context(ast_context)
            {
            }

            [[maybe_unused]] bool shouldVisitTemplateInstantiations() const
            {
                return false;
            }

            [[maybe_unused]] bool shouldWalkTypesOfTypeLocs() const
            {
                return false;
            }

            [[maybe_unused]] bool VisitCXXRecordDecl(clang::CXXRecordDecl* decl)
            {
                if (decl != nullptr and not decl->isUnion())
                {
                    if (cpp_file* cpp_file = get_cpp_file(*decl))
                    {
                        cpp_file->classes.emplace_back(make_class(ast_context, *decl));
                    }
                }

                return true;
            }

            [[maybe_unused]] bool VisitEnumDecl(clang::EnumDecl* decl)
            {
                if (decl != nullptr)
                {
                    if (cpp_file* cpp_file = get_cpp_file(*decl))
                    {
                        cpp_file->enums.emplace_back(make_enum(ast_context, *decl));
                    }
                }

                return true;
            }

            [[maybe_unused]] bool VisitFunctionDecl(clang::FunctionDecl* decl)
            {
                if (decl != nullptr and not decl->isImplicit() and not decl->isTemplateInstantiation())
                {
                    const clang::DeclContext* context = decl->getDeclContext();
                    const clang::NamespaceDecl* namespace_decl = llvm::dyn_cast<clang::NamespaceDecl>(context);

                    if (namespace_decl != nullptr or context->isTranslationUnit())
                    {
                        if (cpp_file* cpp_file = get_cpp_file(*decl))
                        {
                            cpp_file->functions.emplace_back(make_function(ast_context, *decl));
                        }
                    }
                }

                return true;
            }

            [[maybe_unused]] bool VisitVarDecl(clang::VarDecl* decl)
            {
                if (decl != nullptr and not decl->isImplicit())
                {
                    const clang::DeclContext* context = decl->getDeclContext();
                    const clang::NamespaceDecl* namespace_decl = llvm::dyn_cast<clang::NamespaceDecl>(context);

                    if (namespace_decl != nullptr or context->isTranslationUnit())
                    {
                        if (cpp_file* cpp_file = get_cpp_file(*decl))
                        {
                            cpp_file->variables.emplace_back(make_variable(ast_context, *decl));
                        }
                    }
                }

                return true;
            }

            cpp_file* get_cpp_file(clang::Decl& decl)
            {
                clang::SourceManager& source_manager = ast_context.getSourceManager();

                const clang::SourceLocation location = decl.getLocation();
                const clang::FileID file_id = source_manager.getFileID(location);

                if (const clang::FileEntry* file_entry = source_manager.getFileEntryForID(file_id))
                {
                    const std::string file_path = std::filesystem::absolute(file_entry->tryGetRealPathName().str()).string();
                    const auto it_file_map = action_context.cpp_file_map.find(file_path);

                    if (it_file_map != action_context.cpp_file_map.end())
                    {
                        const std::size_t file_index = it_file_map->second;
                        return &action_context.cpp_files.at(file_index);
                    }
                }

                return nullptr;
            }
        };

        struct ast_consumer : clang::ASTConsumer
        {
            frontend_action_context& action_context;

            explicit ast_consumer(frontend_action_context& action_context)
                : action_context(action_context)
            {
            }

            void HandleTranslationUnit(clang::ASTContext& ast_context) override
            {
                ast_visitor visitor {action_context, ast_context};
                visitor.TraverseAST(ast_context);
            }
        };

        struct frontend_action : clang::ASTFrontendAction
        {
            frontend_action_context& action_context;

            explicit frontend_action(frontend_action_context& action_context)
                : action_context(action_context)
            {
            }

            void ExecuteAction() override
            {
                clang::CompilerInstance& compiler_instance = getCompilerInstance();
                compiler_instance.getFrontendOpts().SkipFunctionBodies = true;

                clang::DiagnosticsEngine& diagnostics_engine = compiler_instance.getDiagnostics();
                diagnostics_engine.setSuppressAllDiagnostics(true);

                ASTFrontendAction::ExecuteAction();
            }

            std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler_instance, clang::StringRef file) override
            {
                return std::make_unique<ast_consumer>(action_context);
            }
        };

        struct frontend_action_factory : clang::tooling::FrontendActionFactory
        {
            frontend_action_context& action_context;

            explicit frontend_action_factory(frontend_action_context& action_context)
                : action_context(action_context)
            {
            }

            std::unique_ptr<clang::FrontendAction> create() override
            {
                return std::make_unique<frontend_action>(action_context);
            }
        };
    }

    bool codegen_parser_cpp::parse_asts(const std::vector<std::string>& paths, std::vector<cpp_file>& cpp_files)
    {
        constexpr std::size_t extra_args = 3;

        std::vector<const char*> args;
        args.reserve(paths.size() + additional_args.size() + extra_args);

        const auto transformer = [](const std::string& arg) { return arg.data(); };

        args.emplace_back(SPORE_CODEGEN_NAME);

        std::ranges::transform(paths, std::back_inserter(args), transformer);

        args.emplace_back("--");
        args.emplace_back("--target=" LLVM_HOST_TRIPLE);

        std::ranges::transform(additional_args, std::back_inserter(args), transformer);

        int args_size = args.size();

        llvm::cl::OptionCategory option_category {SPORE_CODEGEN_NAME};
        llvm::Expected<clang::tooling::CommonOptionsParser> options_parser =
            clang::tooling::CommonOptionsParser::create(args_size, args.data(), option_category);

        if (!options_parser)
        {
            SPDLOG_ERROR("invalid clang options: {}", llvm::toString(options_parser.takeError()));
            return false;
        }

        std::string cpp_source;
        std::map<std::string_view, std::size_t> cpp_file_map;

        for (const std::string& cpp_source_file : options_parser->getSourcePathList())
        {
            std::string cpp_source_path = std::filesystem::absolute(cpp_source_file).string();

            cpp_source += std::format("#include \"{}\"\n", cpp_source_path);

            const std::size_t cpp_file_index = cpp_files.size();

            cpp_file& cpp_file = cpp_files.emplace_back();
            cpp_file.path = std::move(cpp_source_path);

            cpp_file_map.emplace(cpp_file.path, cpp_file_index);
        }

        constexpr const char cpp_source_path[] = "__source__.cpp";

        clang::tooling::ClangTool clang_tool {options_parser->getCompilations(), {std::string {cpp_source_path}}};
        clang_tool.mapVirtualFile(cpp_source_path, cpp_source);
        clang_tool.setPrintErrorMessage(false);

        detail::frontend_action_context action_context {cpp_files, cpp_file_map};
        detail::frontend_action_factory action_factory {action_context};

        const int action_result = clang_tool.run(&action_factory);
        return action_result == 0;
    }
}
