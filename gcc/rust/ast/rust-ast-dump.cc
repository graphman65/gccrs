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

#include "rust-ast-dump.h"

namespace Rust {
namespace AST {

Indent::Indent () : tabs (0) {}

std::ostream &
operator<< (std::ostream &stream, const Indent &indent)
{
  return stream << std::string (indent.tabs, '\t');
}

void
Indent::increment ()
{
  tabs++;
}

void
Indent::decrement ()
{
  rust_assert (tabs != 0);
  tabs--;
}

Dump::Dump (std::ostream &stream) : stream (stream), indentation (Indent ()) {}

void
Dump::go (AST::Crate &crate)
{
  for (auto &item : crate.items)
    item->accept_vis (*this);
}

void
Dump::format_function_param (FunctionParam &param)
{
  param.get_pattern ()->accept_vis (*this);
  stream << ": ";
  param.get_type ()->accept_vis (*this);
}

void
Dump::visit (Token &tok)
{}

void
Dump::visit (TokensTree &tok_tree)
{}

void
Dump::visit (AttrInputMetaItemContainer &input)
{}

void
Dump::visit (IdentifierExpr &ident_expr)
{}

void
Dump::visit (Lifetime &lifetime)
{}

void
Dump::visit (LifetimeParam &lifetime_param)
{}

void
Dump::visit (ConstGenericParam &lifetime_param)
{}

// rust-path.h
void
Dump::visit (PathInExpression &path)
{}

void
Dump::visit (TypePathSegment &segment)
{}

void
Dump::visit (TypePathSegmentGeneric &segment)
{}

void
Dump::visit (TypePathSegmentFunction &segment)
{}

void
Dump::visit (TypePath &path)
{
  stream << path.as_string ();
}

void
Dump::visit (QualifiedPathInExpression &path)
{}

void
Dump::visit (QualifiedPathInType &path)
{}

// rust-expr.h
void
Dump::visit (LiteralExpr &expr)
{}

void
Dump::visit (AttrInputLiteral &attr_input)
{}

void
Dump::visit (MetaItemLitExpr &meta_item)
{}

void
Dump::visit (MetaItemPathLit &meta_item)
{}

void
Dump::visit (BorrowExpr &expr)
{}

void
Dump::visit (DereferenceExpr &expr)
{}

void
Dump::visit (ErrorPropagationExpr &expr)
{}

void
Dump::visit (NegationExpr &expr)
{}

void
Dump::visit (ArithmeticOrLogicalExpr &expr)
{
  stream << "arithmetic or logical expression";
}

void
Dump::visit (ComparisonExpr &expr)
{}

void
Dump::visit (LazyBooleanExpr &expr)
{}

void
Dump::visit (TypeCastExpr &expr)
{}

void
Dump::visit (AssignmentExpr &expr)
{}

void
Dump::visit (CompoundAssignmentExpr &expr)
{}

void
Dump::visit (GroupedExpr &expr)
{}

void
Dump::visit (ArrayElemsValues &elems)
{}

void
Dump::visit (ArrayElemsCopied &elems)
{}

void
Dump::visit (ArrayExpr &expr)
{}

void
Dump::visit (ArrayIndexExpr &expr)
{}

void
Dump::visit (TupleExpr &expr)
{}

void
Dump::visit (TupleIndexExpr &expr)
{}

void
Dump::visit (StructExprStruct &expr)
{}

void
Dump::visit (StructExprFieldIdentifier &field)
{}

void
Dump::visit (StructExprFieldIdentifierValue &field)
{}

void
Dump::visit (StructExprFieldIndexValue &field)
{}

void
Dump::visit (StructExprStructFields &expr)
{}

void
Dump::visit (StructExprStructBase &expr)
{}

void
Dump::visit (CallExpr &expr)
{}

void
Dump::visit (MethodCallExpr &expr)
{}

void
Dump::visit (FieldAccessExpr &expr)
{}

void
Dump::visit (ClosureExprInner &expr)
{}

void
Dump::visit (BlockExpr &expr)
{
  stream << "{\n";
  indentation.increment ();

  for (auto &stmt : expr.get_statements ())
    {
      stream << indentation;
      stmt->accept_vis (*this);
      stream << ";\n";
    }

  if (expr.has_tail_expr ())
    expr.get_tail_expr ()->accept_vis (*this);

  indentation.decrement ();
  stream << "\n" << indentation << "}\n";
}

void
Dump::visit (ClosureExprInnerTyped &expr)
{}

void
Dump::visit (ContinueExpr &expr)
{}

void
Dump::visit (BreakExpr &expr)
{}

void
Dump::visit (RangeFromToExpr &expr)
{}

void
Dump::visit (RangeFromExpr &expr)
{}

void
Dump::visit (RangeToExpr &expr)
{}

void
Dump::visit (RangeFullExpr &expr)
{}

void
Dump::visit (RangeFromToInclExpr &expr)
{}

void
Dump::visit (RangeToInclExpr &expr)
{}

void
Dump::visit (ReturnExpr &expr)
{}

void
Dump::visit (UnsafeBlockExpr &expr)
{}

void
Dump::visit (LoopExpr &expr)
{}

void
Dump::visit (WhileLoopExpr &expr)
{}

void
Dump::visit (WhileLetLoopExpr &expr)
{}

void
Dump::visit (ForLoopExpr &expr)
{}

void
Dump::visit (IfExpr &expr)
{}

void
Dump::visit (IfExprConseqElse &expr)
{}

void
Dump::visit (IfExprConseqIf &expr)
{}

void
Dump::visit (IfExprConseqIfLet &expr)
{}

void
Dump::visit (IfLetExpr &expr)
{}

void
Dump::visit (IfLetExprConseqElse &expr)
{}

void
Dump::visit (IfLetExprConseqIf &expr)
{}

void
Dump::visit (IfLetExprConseqIfLet &expr)
{}

void
Dump::visit (MatchExpr &expr)
{}

void
Dump::visit (AwaitExpr &expr)
{}

void
Dump::visit (AsyncBlockExpr &expr)
{}

// rust-item.h
void
Dump::visit (TypeParam &param)
{
  // Is it possible to have a null type here?
  param.get_type ()->accept_vis (*this);
}

void
Dump::visit (LifetimeWhereClauseItem &item)
{}

void
Dump::visit (TypeBoundWhereClauseItem &item)
{}

void
Dump::visit (Method &method)
{
  stream << indentation << "fn " << method.get_method_name () << '(';

  auto &self = method.get_self_param ();
  stream << self.as_string ();

  auto &params = method.get_function_params ();
  for (auto &param : params)
    {
      stream << ", ";
      format_function_param (param);
    }

  stream << ") ";

  if (method.has_return_type ())
    {
      stream << "-> ";
      method.get_return_type ()->accept_vis (*this);
      stream << " ";
    }

  auto &block = method.get_definition ();
  if (!block)
    stream << ';';
  else
    block->accept_vis (*this);

  stream << '\n';
}

void
Dump::visit (Module &module)
{}

void
Dump::visit (ExternCrate &crate)
{}

void
Dump::visit (UseTreeGlob &use_tree)
{}

void
Dump::visit (UseTreeList &use_tree)
{}

void
Dump::visit (UseTreeRebind &use_tree)
{}

void
Dump::visit (UseDeclaration &use_decl)
{}

void
Dump::visit (Function &function)
{
  stream << indentation << "fn " << function.get_function_name () << '(';

  auto &params = function.get_function_params ();
  if (params.size () >= 1)
    {
      format_function_param (params[0]);
      for (size_t i = 1; i < params.size (); i++)
	{
	  stream << ", ";
	  format_function_param (params[i]);
	}
    }

  stream << ") ";

  if (function.has_return_type ())
    {
      stream << "-> ";
      function.get_return_type ()->accept_vis (*this);
      stream << " ";
    }

  auto &block = function.get_definition ();
  if (!block)
    stream << ';';
  else
    block->accept_vis (*this);

  stream << '\n';
}

void
Dump::visit (TypeAlias &type_alias)
{}

void
Dump::visit (StructStruct &struct_item)
{}

void
Dump::visit (TupleStruct &tuple_struct)
{}

void
Dump::visit (EnumItem &item)
{}

void
Dump::visit (EnumItemTuple &item)
{}

void
Dump::visit (EnumItemStruct &item)
{}

void
Dump::visit (EnumItemDiscriminant &item)
{}

void
Dump::visit (Enum &enum_item)
{}

void
Dump::visit (Union &union_item)
{}

void
Dump::visit (ConstantItem &const_item)
{}

void
Dump::visit (StaticItem &static_item)
{}

void
Dump::format_function_common (std::unique_ptr<Type> &return_type,
			      std::unique_ptr<BlockExpr> &block)
{
  if (return_type)
    {
      stream << "-> ";
      return_type->accept_vis (*this);
    }

  if (block)
    {
      if (return_type)
	stream << ' ';
      block->accept_vis (*this);
    }
  else
    stream << ";\n";
}

void
Dump::visit (TraitItemFunc &item)
{
  auto func = item.get_trait_function_decl ();
  stream << indentation << "fn " << func.get_identifier () << '(';

  auto &params = func.get_function_params ();
  for (auto &param : params)
    {
      stream << ", ";
      format_function_param (param);
    }

  stream << ") ";

  format_function_common (func.get_return_type (), item.get_definition ());
}

void
Dump::visit (TraitItemMethod &item)
{
  auto method = item.get_trait_method_decl ();
  stream << indentation << "fn " << method.get_identifier () << '(';

  auto &self = method.get_self_param ();
  stream << self.as_string ();

  auto &params = method.get_function_params ();
  for (auto &param : params)
    {
      stream << ", ";
      format_function_param (param);
    }

  stream << ") ";

  format_function_common (method.get_return_type (), item.get_definition ());
}

void
Dump::visit (TraitItemConst &item)
{
  stream << indentation << "const " << item.get_identifier () << ": ";
  item.get_type ()->accept_vis (*this);
  stream << ";\n";
}

void
Dump::visit (TraitItemType &item)
{
  stream << indentation << "type " << item.get_identifier () << ";\n";
}

void
Dump::visit (Trait &trait)
{
  stream << "trait " << trait.get_identifier () << " {\n";
  indentation.increment ();

  for (auto &item : trait.get_trait_items ())
    item->accept_vis (*this);

  indentation.decrement ();
  stream << "\n}\n";
}

void
Dump::visit (InherentImpl &impl)
{
  stream << "impl ";

  // FIXME: Handle generics

  impl.get_type ()->accept_vis (*this);

  // FIXME: Handle where-clause
  // FIXME: Handle inner attributes

  stream << " {\n";
  indentation.increment ();

  for (auto &item : impl.get_impl_items ())
    item->accept_vis (*this);

  indentation.decrement ();
  stream << "\n}\n";
}

void
Dump::visit (TraitImpl &impl)
{
  stream << "impl ";
  impl.get_trait_path ().accept_vis (*this);
  stream << " for ";
  impl.get_type ()->accept_vis (*this);

  stream << " {\n";
  indentation.increment ();

  for (auto &item : impl.get_impl_items ())
    item->accept_vis (*this);

  indentation.decrement ();
  stream << "\n}\n";
}

void
Dump::visit (ExternalStaticItem &item)
{}

void
Dump::visit (ExternalFunctionItem &item)
{}

void
Dump::visit (ExternBlock &block)
{}

// rust-macro.h
void
Dump::visit (MacroMatchFragment &match)
{}

void
Dump::visit (MacroMatchRepetition &match)
{}

void
Dump::visit (MacroMatcher &matcher)
{}

void
Dump::visit (MacroRulesDefinition &rules_def)
{}

void
Dump::visit (MacroInvocation &macro_invoc)
{}

void
Dump::visit (MetaItemPath &meta_item)
{}

void
Dump::visit (MetaItemSeq &meta_item)
{}

void
Dump::visit (MetaWord &meta_item)
{}

void
Dump::visit (MetaNameValueStr &meta_item)
{}

void
Dump::visit (MetaListPaths &meta_item)
{}

void
Dump::visit (MetaListNameValueStr &meta_item)
{}

// rust-pattern.h
void
Dump::visit (LiteralPattern &pattern)
{}

void
Dump::visit (IdentifierPattern &pattern)
{
  stream << pattern.get_ident ();
}

void
Dump::visit (WildcardPattern &pattern)
{}

// void Dump::visit(RangePatternBound& bound){}

void
Dump::visit (RangePatternBoundLiteral &bound)
{}

void
Dump::visit (RangePatternBoundPath &bound)
{}

void
Dump::visit (RangePatternBoundQualPath &bound)
{}

void
Dump::visit (RangePattern &pattern)
{}

void
Dump::visit (ReferencePattern &pattern)
{}

// void Dump::visit(StructPatternField& field){}

void
Dump::visit (StructPatternFieldTuplePat &field)
{}

void
Dump::visit (StructPatternFieldIdentPat &field)
{}

void
Dump::visit (StructPatternFieldIdent &field)
{}

void
Dump::visit (StructPattern &pattern)
{}

// void Dump::visit(TupleStructItems& tuple_items){}

void
Dump::visit (TupleStructItemsNoRange &tuple_items)
{}

void
Dump::visit (TupleStructItemsRange &tuple_items)
{}

void
Dump::visit (TupleStructPattern &pattern)
{}

// void Dump::visit(TuplePatternItems& tuple_items){}

void
Dump::visit (TuplePatternItemsMultiple &tuple_items)
{}

void
Dump::visit (TuplePatternItemsRanged &tuple_items)
{}

void
Dump::visit (TuplePattern &pattern)
{}

void
Dump::visit (GroupedPattern &pattern)
{}

void
Dump::visit (SlicePattern &pattern)
{}

// rust-stmt.h
void
Dump::visit (EmptyStmt &stmt)
{}

void
Dump::visit (LetStmt &stmt)
{
  stream << "let ";
  auto &pattern = stmt.get_pattern ();
  if (pattern)
    pattern->accept_vis (*this);

  if (stmt.has_type ())
    {
      stream << ": ";
      stmt.get_type ()->accept_vis (*this);
    }

  if (stmt.has_init_expr ())
    {
      stream << " = ";
      stmt.get_init_expr ()->accept_vis (*this);
    }
}

void
Dump::visit (ExprStmtWithoutBlock &stmt)
{}

void
Dump::visit (ExprStmtWithBlock &stmt)
{}

// rust-type.h
void
Dump::visit (TraitBound &bound)
{}

void
Dump::visit (ImplTraitType &type)
{}

void
Dump::visit (TraitObjectType &type)
{}

void
Dump::visit (ParenthesisedType &type)
{}

void
Dump::visit (ImplTraitTypeOneBound &type)
{}

void
Dump::visit (TraitObjectTypeOneBound &type)
{}

void
Dump::visit (TupleType &type)
{}

void
Dump::visit (NeverType &type)
{}

void
Dump::visit (RawPointerType &type)
{}

void
Dump::visit (ReferenceType &type)
{
  type.get_type_referenced ()->accept_vis (*this);
}

void
Dump::visit (ArrayType &type)
{
  type.get_elem_type ()->accept_vis (*this);
}

void
Dump::visit (SliceType &type)
{
  type.get_elem_type ()->accept_vis (*this);
}

void
Dump::visit (InferredType &type)
{
  stream << "_";
}

void
Dump::visit (BareFunctionType &type)
{}

} // namespace AST
} // namespace Rust
