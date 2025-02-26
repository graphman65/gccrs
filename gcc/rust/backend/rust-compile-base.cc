// Copyright (C) 2020-2022 Free Software Foundation, Inc.

// This file is part of GCC.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include "rust-compile-base.h"
#include "rust-compile-item.h"
#include "rust-compile-stmt.h"
#include "rust-compile-fnparam.h"
#include "rust-compile-var-decl.h"

#include "rust-expr.h" // for AST::AttrInputLiteral

#include "fold-const.h"
#include "stringpool.h"
#include "attribs.h"

namespace Rust {
namespace Compile {

bool inline should_mangle_item (const tree fndecl)
{
  return lookup_attribute ("no_mangle", DECL_ATTRIBUTES (fndecl)) == NULL_TREE;
}

void
HIRCompileBase::setup_fndecl (tree fndecl, bool is_main_entry_point,
			      bool is_generic_fn, HIR::Visibility &visibility,
			      const HIR::FunctionQualifiers &qualifiers,
			      const AST::AttrVec &attrs)
{
  // if its the main fn or pub visibility mark its as DECL_PUBLIC
  // please see https://github.com/Rust-GCC/gccrs/pull/137
  bool is_pub = visibility.get_vis_type () == HIR::Visibility::VisType::PUBLIC;
  if (is_main_entry_point || (is_pub && !is_generic_fn))
    {
      TREE_PUBLIC (fndecl) = 1;
    }

  // is it a const fn
  if (qualifiers.is_const ())
    {
      TREE_READONLY (fndecl) = 1;
    }

  // is it inline?
  for (const auto &attr : attrs)
    {
      bool is_inline = attr.get_path ().as_string ().compare ("inline") == 0;
      bool is_must_use
	= attr.get_path ().as_string ().compare ("must_use") == 0;
      bool is_cold = attr.get_path ().as_string ().compare ("cold") == 0;
      bool is_link_section
	= attr.get_path ().as_string ().compare ("link_section") == 0;
      bool no_mangle = attr.get_path ().as_string ().compare ("no_mangle") == 0;
      if (is_inline)
	{
	  handle_inline_attribute_on_fndecl (fndecl, attr);
	}
      else if (is_must_use)
	{
	  handle_must_use_attribute_on_fndecl (fndecl, attr);
	}
      else if (is_cold)
	{
	  handle_cold_attribute_on_fndecl (fndecl, attr);
	}
      else if (is_link_section)
	{
	  handle_link_section_attribute_on_fndecl (fndecl, attr);
	}
      else if (no_mangle)
	{
	  handle_no_mangle_attribute_on_fndecl (fndecl, attr);
	}
    }
}

void
HIRCompileBase::handle_cold_attribute_on_fndecl (tree fndecl,
						 const AST::Attribute &attr)
{
  // simple #[cold]
  if (!attr.has_attr_input ())
    {
      tree cold = get_identifier ("cold");
      // this will get handled by the GCC backend later
      DECL_ATTRIBUTES (fndecl)
	= tree_cons (cold, NULL_TREE, DECL_ATTRIBUTES (fndecl));
      return;
    }

  rust_error_at (attr.get_locus (),
		 "attribute %<cold%> does not accept any arguments");
}

void
HIRCompileBase::handle_link_section_attribute_on_fndecl (
  tree fndecl, const AST::Attribute &attr)
{
  if (!attr.has_attr_input ())
    {
      rust_error_at (attr.get_locus (),
		     "%<link_section%> expects exactly one argment");
      return;
    }

  rust_assert (attr.get_attr_input ().get_attr_input_type ()
	       == AST::AttrInput::AttrInputType::LITERAL);

  auto &literal = static_cast<AST::AttrInputLiteral &> (attr.get_attr_input ());
  const auto &msg_str = literal.get_literal ().as_string ();

  if (decl_section_name (fndecl))
    {
      rust_warning_at (attr.get_locus (), 0, "section name redefined");
    }

  set_decl_section_name (fndecl, msg_str.c_str ());
}

void
HIRCompileBase::handle_no_mangle_attribute_on_fndecl (
  tree fndecl, const AST::Attribute &attr)
{
  if (attr.has_attr_input ())
    {
      rust_error_at (attr.get_locus (),
		     "attribute %<no_mangle%> does not accept any arguments");
      return;
    }

  DECL_ATTRIBUTES (fndecl) = tree_cons (get_identifier ("no_mangle"), NULL_TREE,
					DECL_ATTRIBUTES (fndecl));
}

void
HIRCompileBase::handle_inline_attribute_on_fndecl (tree fndecl,
						   const AST::Attribute &attr)
{
  // simple #[inline]
  if (!attr.has_attr_input ())
    {
      DECL_DECLARED_INLINE_P (fndecl) = 1;
      return;
    }

  const AST::AttrInput &input = attr.get_attr_input ();
  bool is_token_tree
    = input.get_attr_input_type () == AST::AttrInput::AttrInputType::TOKEN_TREE;
  rust_assert (is_token_tree);
  const auto &option = static_cast<const AST::DelimTokenTree &> (input);
  AST::AttrInputMetaItemContainer *meta_item = option.parse_to_meta_item ();
  if (meta_item->get_items ().size () != 1)
    {
      rust_error_at (attr.get_locus (), "invalid number of arguments");
      return;
    }

  const std::string inline_option
    = meta_item->get_items ().at (0)->as_string ();

  // we only care about NEVER and ALWAYS else its an error
  bool is_always = inline_option.compare ("always") == 0;
  bool is_never = inline_option.compare ("never") == 0;

  // #[inline(never)]
  if (is_never)
    {
      DECL_UNINLINABLE (fndecl) = 1;
    }
  // #[inline(always)]
  else if (is_always)
    {
      DECL_DECLARED_INLINE_P (fndecl) = 1;
      DECL_ATTRIBUTES (fndecl) = tree_cons (get_identifier ("always_inline"),
					    NULL, DECL_ATTRIBUTES (fndecl));
    }
  else
    {
      rust_error_at (attr.get_locus (), "unknown inline option");
    }
}

void
HIRCompileBase::handle_must_use_attribute_on_fndecl (tree fndecl,
						     const AST::Attribute &attr)
{
  tree nodiscard = get_identifier ("nodiscard");
  tree value = NULL_TREE;

  if (attr.has_attr_input ())
    {
      rust_assert (attr.get_attr_input ().get_attr_input_type ()
		   == AST::AttrInput::AttrInputType::LITERAL);

      auto &literal
	= static_cast<AST::AttrInputLiteral &> (attr.get_attr_input ());
      const auto &msg_str = literal.get_literal ().as_string ();
      tree message = build_string (msg_str.size (), msg_str.c_str ());

      value = tree_cons (nodiscard, message, NULL_TREE);
    }

  DECL_ATTRIBUTES (fndecl)
    = tree_cons (nodiscard, value, DECL_ATTRIBUTES (fndecl));
}

void
HIRCompileBase::setup_abi_options (tree fndecl, ABI abi)
{
  switch (abi)
    {
    case Rust::ABI::RUST:
    case Rust::ABI::INTRINSIC:
    case Rust::ABI::C:
    case Rust::ABI::CDECL:
      DECL_ATTRIBUTES (fndecl)
	= tree_cons (get_identifier ("cdecl"), NULL, DECL_ATTRIBUTES (fndecl));
      break;

    case Rust::ABI::STDCALL:
      DECL_ATTRIBUTES (fndecl) = tree_cons (get_identifier ("stdcall"), NULL,
					    DECL_ATTRIBUTES (fndecl));
      break;

    case Rust::ABI::FASTCALL:
      DECL_ATTRIBUTES (fndecl) = tree_cons (get_identifier ("fastcall"), NULL,
					    DECL_ATTRIBUTES (fndecl));

      break;

    default:
      break;
    }
}

// ported from gcc/c/c-typecheck.c
//
// Mark EXP saying that we need to be able to take the
// address of it; it should not be allocated in a register.
// Returns true if successful.  ARRAY_REF_P is true if this
// is for ARRAY_REF construction - in that case we don't want
// to look through VIEW_CONVERT_EXPR from VECTOR_TYPE to ARRAY_TYPE,
// it is fine to use ARRAY_REFs for vector subscripts on vector
// register variables.
bool
HIRCompileBase::mark_addressable (tree exp, Location locus)
{
  tree x = exp;

  while (1)
    switch (TREE_CODE (x))
      {
      case VIEW_CONVERT_EXPR:
	if (TREE_CODE (TREE_TYPE (x)) == ARRAY_TYPE
	    && VECTOR_TYPE_P (TREE_TYPE (TREE_OPERAND (x, 0))))
	  return true;
	x = TREE_OPERAND (x, 0);
	break;

      case COMPONENT_REF:
	// TODO
	// if (DECL_C_BIT_FIELD (TREE_OPERAND (x, 1)))
	//   {
	//     error ("cannot take address of bit-field %qD", TREE_OPERAND (x,
	//     1)); return false;
	//   }

	/* FALLTHRU */
      case ADDR_EXPR:
      case ARRAY_REF:
      case REALPART_EXPR:
      case IMAGPART_EXPR:
	x = TREE_OPERAND (x, 0);
	break;

      case COMPOUND_LITERAL_EXPR:
	TREE_ADDRESSABLE (x) = 1;
	TREE_ADDRESSABLE (COMPOUND_LITERAL_EXPR_DECL (x)) = 1;
	return true;

      case CONSTRUCTOR:
	TREE_ADDRESSABLE (x) = 1;
	return true;

      case VAR_DECL:
      case CONST_DECL:
      case PARM_DECL:
      case RESULT_DECL:
	// (we don't have a concept of a "register" declaration)
	// fallthrough */

	/* FALLTHRU */
      case FUNCTION_DECL:
	TREE_ADDRESSABLE (x) = 1;

	/* FALLTHRU */
      default:
	return true;
      }

  return false;
}

tree
HIRCompileBase::address_expression (tree expr, tree ptrtype, Location location)
{
  if (expr == error_mark_node)
    return error_mark_node;

  if (!mark_addressable (expr, location))
    return error_mark_node;

  return build_fold_addr_expr_with_type_loc (location.gcc_location (), expr,
					     ptrtype);
}

std::vector<Bvariable *>
HIRCompileBase::compile_locals_for_block (Context *ctx, Resolver::Rib &rib,
					  tree fndecl)
{
  CrateNum crate = ctx->get_mappings ()->get_current_crate ();

  std::vector<Bvariable *> locals;
  for (auto it : rib.get_declarations ())
    {
      NodeId node_id = it.first;
      HirId ref = UNKNOWN_HIRID;
      if (!ctx->get_mappings ()->lookup_node_to_hir (crate, node_id, &ref))
	continue;

      // we only care about local patterns
      HIR::Pattern *pattern
	= ctx->get_mappings ()->lookup_hir_pattern (crate, ref);
      if (pattern == nullptr)
	continue;

      // lookup the type
      TyTy::BaseType *tyty = nullptr;
      if (!ctx->get_tyctx ()->lookup_type (ref, &tyty))
	continue;

      // compile the local
      tree type = TyTyResolveCompile::compile (ctx, tyty);
      Bvariable *compiled
	= CompileVarDecl::compile (fndecl, type, pattern, ctx);
      locals.push_back (compiled);
    }
  return locals;
}

void
HIRCompileBase::compile_function_body (Context *ctx, tree fndecl,
				       HIR::BlockExpr &function_body,
				       bool has_return_type)
{
  for (auto &s : function_body.get_statements ())
    {
      auto compiled_expr = CompileStmt::Compile (s.get (), ctx);
      if (compiled_expr != nullptr)
	{
	  tree s = convert_to_void (compiled_expr, ICV_STATEMENT);
	  ctx->add_statement (s);
	}
    }

  if (function_body.has_expr ())
    {
      // the previous passes will ensure this is a valid return
      // or a valid trailing expression
      tree compiled_expr
	= CompileExpr::Compile (function_body.expr.get (), ctx);

      if (compiled_expr != nullptr)
	{
	  if (has_return_type)
	    {
	      std::vector<tree> retstmts;
	      retstmts.push_back (compiled_expr);

	      auto ret = ctx->get_backend ()->return_statement (
		fndecl, retstmts,
		function_body.get_final_expr ()->get_locus ());
	      ctx->add_statement (ret);
	    }
	  else
	    {
	      // FIXME can this actually happen?
	      ctx->add_statement (compiled_expr);
	    }
	}
    }
}

tree
HIRCompileBase::compile_function (
  Context *ctx, const std::string &fn_name, HIR::SelfParam &self_param,
  std::vector<HIR::FunctionParam> &function_params,
  const HIR::FunctionQualifiers &qualifiers, HIR::Visibility &visibility,
  AST::AttrVec &outer_attrs, Location locus, HIR::BlockExpr *function_body,
  const Resolver::CanonicalPath *canonical_path, TyTy::FnType *fntype,
  bool function_has_return)
{
  tree compiled_fn_type = TyTyResolveCompile::compile (ctx, fntype);
  std::string ir_symbol_name
    = canonical_path->get () + fntype->subst_as_string ();

  // we don't mangle the main fn since we haven't implemented the main shim
  bool is_main_fn = fn_name.compare ("main") == 0;
  std::string asm_name = fn_name;

  unsigned int flags = 0;
  tree fndecl = ctx->get_backend ()->function (compiled_fn_type, ir_symbol_name,
					       "" /* asm_name */, flags, locus);

  setup_fndecl (fndecl, is_main_fn, fntype->has_subsititions_defined (),
		visibility, qualifiers, outer_attrs);
  setup_abi_options (fndecl, fntype->get_abi ());

  // conditionally mangle the function name
  bool should_mangle = should_mangle_item (fndecl);
  if (!is_main_fn && should_mangle)
    asm_name = ctx->mangle_item (fntype, *canonical_path);
  SET_DECL_ASSEMBLER_NAME (fndecl,
			   get_identifier_with_length (asm_name.data (),
						       asm_name.length ()));

  // insert into the context
  ctx->insert_function_decl (fntype, fndecl);

  // setup the params
  TyTy::BaseType *tyret = fntype->get_return_type ();
  std::vector<Bvariable *> param_vars;
  if (!self_param.is_error ())
    {
      rust_assert (fntype->is_method ());
      TyTy::BaseType *self_tyty_lookup = fntype->get_self_type ();

      tree self_type = TyTyResolveCompile::compile (ctx, self_tyty_lookup);
      Bvariable *compiled_self_param
	= CompileSelfParam::compile (ctx, fndecl, self_param, self_type,
				     self_param.get_locus ());

      param_vars.push_back (compiled_self_param);
      ctx->insert_var_decl (self_param.get_mappings ().get_hirid (),
			    compiled_self_param);
    }

  // offset from + 1 for the TyTy::FnType being used when this is a method to
  // skip over Self on the FnType
  bool is_method = !self_param.is_error ();
  size_t i = is_method ? 1 : 0;
  for (auto &referenced_param : function_params)
    {
      auto tyty_param = fntype->param_at (i++);
      auto param_tyty = tyty_param.second;
      auto compiled_param_type = TyTyResolveCompile::compile (ctx, param_tyty);

      Location param_locus = referenced_param.get_locus ();
      Bvariable *compiled_param_var
	= CompileFnParam::compile (ctx, fndecl, &referenced_param,
				   compiled_param_type, param_locus);

      param_vars.push_back (compiled_param_var);

      const HIR::Pattern &param_pattern = *referenced_param.get_param_name ();
      ctx->insert_var_decl (param_pattern.get_pattern_mappings ().get_hirid (),
			    compiled_param_var);
    }

  if (!ctx->get_backend ()->function_set_parameters (fndecl, param_vars))
    return error_mark_node;

  // lookup locals
  auto body_mappings = function_body->get_mappings ();
  Resolver::Rib *rib = nullptr;
  bool ok
    = ctx->get_resolver ()->find_name_rib (body_mappings.get_nodeid (), &rib);
  rust_assert (ok);

  std::vector<Bvariable *> locals
    = compile_locals_for_block (ctx, *rib, fndecl);

  tree enclosing_scope = NULL_TREE;
  Location start_location = function_body->get_locus ();
  Location end_location = function_body->get_end_locus ();

  tree code_block = ctx->get_backend ()->block (fndecl, enclosing_scope, locals,
						start_location, end_location);
  ctx->push_block (code_block);

  Bvariable *return_address = nullptr;
  if (function_has_return)
    {
      tree return_type = TyTyResolveCompile::compile (ctx, tyret);

      bool address_is_taken = false;
      tree ret_var_stmt = NULL_TREE;

      return_address
	= ctx->get_backend ()->temporary_variable (fndecl, code_block,
						   return_type, NULL,
						   address_is_taken, locus,
						   &ret_var_stmt);

      ctx->add_statement (ret_var_stmt);
    }

  ctx->push_fn (fndecl, return_address);
  compile_function_body (ctx, fndecl, *function_body, function_has_return);
  tree bind_tree = ctx->pop_block ();

  gcc_assert (TREE_CODE (bind_tree) == BIND_EXPR);
  DECL_SAVED_TREE (fndecl) = bind_tree;

  ctx->pop_fn ();
  ctx->push_function (fndecl);

  return fndecl;
}

tree
HIRCompileBase::compile_constant_item (
  Context *ctx, TyTy::BaseType *resolved_type,
  const Resolver::CanonicalPath *canonical_path, HIR::Expr *const_value_expr,
  Location locus)
{
  const std::string &ident = canonical_path->get ();
  tree type = TyTyResolveCompile::compile (ctx, resolved_type);
  tree const_type = build_qualified_type (type, TYPE_QUAL_CONST);

  bool is_block_expr
    = const_value_expr->get_expression_type () == HIR::Expr::ExprType::Block;

  // compile the expression
  tree folded_expr = error_mark_node;
  if (!is_block_expr)
    {
      tree value = CompileExpr::Compile (const_value_expr, ctx);
      folded_expr = fold_expr (value);
    }
  else
    {
      // in order to compile a block expr we want to reuse as much existing
      // machineary that we already have. This means the best approach is to
      // make a _fake_ function with a block so it can hold onto temps then
      // use our constexpr code to fold it completely or error_mark_node
      Backend::typed_identifier receiver;
      tree compiled_fn_type = ctx->get_backend ()->function_type (
	receiver, {}, {Backend::typed_identifier ("_", const_type, locus)},
	NULL, locus);

      tree fndecl
	= ctx->get_backend ()->function (compiled_fn_type, ident, "", 0, locus);
      TREE_READONLY (fndecl) = 1;

      tree enclosing_scope = NULL_TREE;
      HIR::BlockExpr *function_body
	= static_cast<HIR::BlockExpr *> (const_value_expr);
      Location start_location = function_body->get_locus ();
      Location end_location = function_body->get_end_locus ();

      tree code_block
	= ctx->get_backend ()->block (fndecl, enclosing_scope, {},
				      start_location, end_location);
      ctx->push_block (code_block);

      bool address_is_taken = false;
      tree ret_var_stmt = NULL_TREE;
      Bvariable *return_address
	= ctx->get_backend ()->temporary_variable (fndecl, code_block,
						   const_type, NULL,
						   address_is_taken, locus,
						   &ret_var_stmt);

      ctx->add_statement (ret_var_stmt);
      ctx->push_fn (fndecl, return_address);

      compile_function_body (ctx, fndecl, *function_body, true);
      tree bind_tree = ctx->pop_block ();

      gcc_assert (TREE_CODE (bind_tree) == BIND_EXPR);
      DECL_SAVED_TREE (fndecl) = bind_tree;

      ctx->pop_fn ();

      // lets fold it into a call expr
      tree call = build_call_array_loc (locus.gcc_location (), const_type,
					fndecl, 0, NULL);
      folded_expr = fold_expr (call);
    }

  return named_constant_expression (const_type, ident, folded_expr, locus);
}

tree
HIRCompileBase::named_constant_expression (tree type_tree,
					   const std::string &name,
					   tree const_val, Location location)
{
  if (type_tree == error_mark_node || const_val == error_mark_node)
    return error_mark_node;

  tree name_tree = get_identifier_with_length (name.data (), name.length ());
  tree decl
    = build_decl (location.gcc_location (), CONST_DECL, name_tree, type_tree);
  DECL_INITIAL (decl) = const_val;
  TREE_CONSTANT (decl) = 1;
  TREE_READONLY (decl) = 1;

  rust_preserve_from_gc (decl);
  return decl;
}

} // namespace Compile
} // namespace Rust
