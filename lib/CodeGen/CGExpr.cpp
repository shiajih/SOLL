// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include <llvm/IR/CFG.h>

namespace soll::CodeGen {

class ExprEmitter {
public:
  CodeGenFunction &CGF;
  CodeGenModule &CGM;
  llvm::IRBuilder<llvm::ConstantFolder> &Builder;
  llvm::LLVMContext &VMContext;
  ExprEmitter(CodeGenFunction &CGF)
      : CGF(CGF), CGM(CGF.getCodeGenModule()), Builder(CGF.getBuilder()),
        VMContext(CGF.getLLVMContext()) {}
  ExprValue visit(const Expr *E) {
    if (auto UO = dynamic_cast<const UnaryOperator *>(E)) {
      return visit(UO);
    }
    if (auto BO = dynamic_cast<const BinaryOperator *>(E)) {
      return visit(BO);
    }
    if (auto CE = dynamic_cast<const CallExpr *>(E)) {
      return visit(CE);
    }
    if (auto CE = dynamic_cast<const CastExpr *>(E)) {
      return visit(CE);
    }
    if (auto NE = dynamic_cast<const NewExpr *>(E)) {
      return visit(NE);
    }
    if (auto ME = dynamic_cast<const MemberExpr *>(E)) {
      return visit(ME);
    }
    if (auto IA = dynamic_cast<const IndexAccess *>(E)) {
      return visit(IA);
    }
    if (auto PE = dynamic_cast<const ParenExpr *>(E)) {
      return visit(PE);
    }
    if (auto CE = dynamic_cast<const ConstantExpr *>(E)) {
      return visit(CE);
    }
    if (auto I = dynamic_cast<const Identifier *>(E)) {
      return visit(I);
    }
    if (auto ETNE = dynamic_cast<const ElementaryTypeNameExpr *>(E)) {
      return visit(ETNE);
    }
    if (auto BL = dynamic_cast<const BooleanLiteral *>(E)) {
      return visit(BL);
    }
    if (auto SL = dynamic_cast<const StringLiteral *>(E)) {
      return visit(SL);
    }
    if (auto NL = dynamic_cast<const NumberLiteral *>(E)) {
      return visit(NL);
    }
    if (auto AI = dynamic_cast<const AsmIdentifier *>(E)) {
      return visit(AI);
    }
    assert(false && "unknown Expr!");
    __builtin_unreachable();
  }

private:
  ExprValue visit(const UnaryOperator *UO) {
    const Expr *SubExpr = UO->getSubExpr();
    ExprValue SubVal = visit(SubExpr);
    llvm::Value *Value = SubVal.load(Builder, CGM);
    if (UO->isArithmeticOp()) {
      switch (UO->getOpcode()) {
      case UnaryOperatorKind::UO_Plus:
        break;
      case UnaryOperatorKind::UO_Minus:
        Value = Builder.CreateNeg(Value);
        break;
      case UnaryOperatorKind::UO_Not:
        Value = Builder.CreateNot(Value);
        break;
      case UnaryOperatorKind::UO_LNot:
        Value = Builder.CreateICmpEQ(Value, Builder.getInt1(false));
        break;
      case UnaryOperatorKind::UO_IsZero:
        Value = Builder.CreateICmpEQ(
            Value, llvm::ConstantInt::get(Value->getType(), 0));
        break;
      default:
        assert(false && "unknown op");
        __builtin_unreachable();
      }
    } else {
      switch (UO->getOpcode()) {
      case UnaryOperatorKind::UO_PostInc:
        SubVal.store(Builder, CGM,
                     Builder.CreateAdd(
                         Value, llvm::ConstantInt::get(Value->getType(), 1)));
        break;
      case UnaryOperatorKind::UO_PostDec:
        SubVal.store(Builder, CGM,
                     Builder.CreateSub(
                         Value, llvm::ConstantInt::get(Value->getType(), 1)));
        break;
      case UnaryOperatorKind::UO_PreInc:
        Value = Builder.CreateAdd(Value,
                                  llvm::ConstantInt::get(Value->getType(), 1));
        SubVal.store(Builder, CGM, Value);
        break;
      case UnaryOperatorKind::UO_PreDec:
        Value = Builder.CreateSub(Value,
                                  llvm::ConstantInt::get(Value->getType(), 1));
        SubVal.store(Builder, CGM, Value);
        break;
      default:
        assert(false && "unknown op");
        __builtin_unreachable();
      }
    }
    return ExprValue::getRValue(SubExpr, Value);
  }

  static bool isSigned(const Type *Ty) {
    if (auto TyNow = dynamic_cast<const IntegerType *>(Ty))
      return TyNow->isSigned();
    else if (dynamic_cast<const BooleanType *>(Ty))
      return false;
    else if (dynamic_cast<const AddressType *>(Ty))
      return false;
    else {
      assert(false && "Wrong type in binary operator!");
      __builtin_unreachable();
    }
  }

  ExprValue visit(const BinaryOperator *BO) {
    const Type *Ty = BO->getType().get();
    llvm::Value *V = nullptr;
    if (BO->isAssignmentOp()) {
      ExprValue LHSVar = visit(BO->getLHS());
      ExprValue RHSVar = visit(BO->getRHS());
      llvm::Value *RHSVal = RHSVar.load(Builder, CGM);
      if (BO->getOpcode() == BO_Assign) {
        LHSVar.store(Builder, CGM, RHSVal);
      } else {
        const bool Signed = isSigned(Ty);
        llvm::Value *LHSVal = LHSVar.load(Builder, CGM);
        switch (BO->getOpcode()) {
        case BO_MulAssign:
          LHSVal = Builder.CreateMul(LHSVal, RHSVal, "BO_MulAssign");
          break;
        case BO_DivAssign:
          if (Signed) {
            LHSVal = Builder.CreateSDiv(LHSVal, RHSVal, "BO_DivAssign");
          } else {
            LHSVal = Builder.CreateUDiv(LHSVal, RHSVal, "BO_DivAssign");
          }
          break;
        case BO_RemAssign:
          if (Signed) {
            LHSVal = Builder.CreateSRem(LHSVal, RHSVal, "BO_RemAssign");
          } else {
            LHSVal = Builder.CreateURem(LHSVal, RHSVal, "BO_RemAssign");
          }
          break;
        case BO_AddAssign:
          LHSVal = Builder.CreateAdd(LHSVal, RHSVal, "BO_AddAssign");
          break;
        case BO_SubAssign:
          LHSVal = Builder.CreateSub(LHSVal, RHSVal, "BO_SubAssign");
          break;
        case BO_ShlAssign:
          LHSVal = Builder.CreateShl(LHSVal, RHSVal, "BO_ShlAssign");
          break;
        case BO_ShrAssign:
          if (Signed) {
            LHSVal = Builder.CreateAShr(LHSVal, RHSVal, "BO_ShrAssign");
          } else {
            LHSVal = Builder.CreateLShr(LHSVal, RHSVal, "BO_ShrAssign");
          }
          break;
        case BO_AndAssign:
          LHSVal = Builder.CreateAnd(LHSVal, RHSVal, "BO_AndAssign");
          break;
        case BO_XorAssign:
          LHSVal = Builder.CreateXor(LHSVal, RHSVal, "BO_XorAssign");
          break;
        case BO_OrAssign:
          LHSVal = Builder.CreateOr(LHSVal, RHSVal, "BO_OrAssign");
          break;
        default:;
        }
        LHSVar.store(Builder, CGM, LHSVal);
      }
      V = RHSVal;
    } else if (BO->isAdditiveOp() || BO->isMultiplicativeOp() ||
               BO->isComparisonOp() || BO->isShiftOp() || BO->isBitwiseOp() ||
               BO->isAsmBitwiseOp() || BO->getOpcode() == BO_Exp) {
      const bool Signed = isSigned(Ty);

      using Pred = llvm::CmpInst::Predicate;
      llvm::Value *LHS = visit(BO->getLHS()).load(Builder, CGM);
      llvm::Value *RHS = visit(BO->getRHS()).load(Builder, CGM);
      switch (BO->getOpcode()) {
      case BinaryOperatorKind::BO_Add:
        V = Builder.CreateAdd(LHS, RHS, "BO_ADD");
        break;
      case BinaryOperatorKind::BO_Sub:
        V = Builder.CreateSub(LHS, RHS, "BO_SUB");
        break;
      case BinaryOperatorKind::BO_Mul:
        V = Builder.CreateMul(LHS, RHS, "BO_MUL");
        break;
      case BinaryOperatorKind::BO_Div:
        if (Signed) {
          V = Builder.CreateSDiv(LHS, RHS, "BO_DIV");
        } else {
          V = Builder.CreateUDiv(LHS, RHS, "BO_DIV");
        }
        break;
      case BinaryOperatorKind::BO_Rem:
        if (Signed) {
          V = Builder.CreateSRem(LHS, RHS, "BO_Rem");
        } else {
          V = Builder.CreateURem(LHS, RHS, "BO_Rem");
        }
        break;
      case BinaryOperatorKind::BO_GE:
        V = Builder.CreateICmp(Pred(Pred::ICMP_UGE + (Signed << 2)), LHS, RHS,
                               "BO_GE");
        break;
      case BinaryOperatorKind::BO_GT:
        V = Builder.CreateICmp(Pred(Pred::ICMP_UGT + (Signed << 2)), LHS, RHS,
                               "BO_GT");
        break;
      case BinaryOperatorKind::BO_LE:
        V = Builder.CreateICmp(Pred(Pred::ICMP_ULE + (Signed << 2)), LHS, RHS,
                               "BO_LE");
        break;
      case BinaryOperatorKind::BO_LT:
        V = Builder.CreateICmp(Pred(Pred::ICMP_ULT + (Signed << 2)), LHS, RHS,
                               "BO_LT");
        break;
      case BinaryOperatorKind::BO_EQ:
        V = Builder.CreateICmpEQ(LHS, RHS, "BO_EQ");
        break;
      case BinaryOperatorKind::BO_NE:
        V = Builder.CreateICmpNE(LHS, RHS, "BO_NE");
        break;
      case BinaryOperatorKind::BO_Shl:
        V = Builder.CreateShl(LHS, RHS, "BO_Shl");
        break;
      case BinaryOperatorKind::BO_Shr:
        if (Signed) {
          V = Builder.CreateAShr(LHS, RHS, "BO_Shr");
        } else {
          V = Builder.CreateLShr(LHS, RHS, "BO_Shr");
        }
        break;
      case BinaryOperatorKind::BO_And:
      case BinaryOperatorKind::BO_AsmAnd:
        V = Builder.CreateAnd(LHS, RHS, "BO_And");
        break;
      case BinaryOperatorKind::BO_Xor:
      case BinaryOperatorKind::BO_AsmXor:
        V = Builder.CreateXor(LHS, RHS, "BO_Xor");
        break;
      case BinaryOperatorKind::BO_Or:
      case BinaryOperatorKind::BO_AsmOr:
        V = Builder.CreateOr(LHS, RHS, "BO_Or");
        break;
      case BinaryOperatorKind::BO_Exp:
        V = CGM.emitExp(LHS, RHS, Signed);
        break;
      default:
        assert(false);
        __builtin_unreachable();
      }
    } else if (BO->isLogicalOp()) {
      if (BO->getOpcode() == BO_LAnd) {
        llvm::BasicBlock *ContBlock = CGF.createBasicBlock("land.end");
        llvm::BasicBlock *RHSBlock = CGF.createBasicBlock("land.rhs");
        CGF.emitBranchOnBoolExpr(BO->getLHS(), RHSBlock, ContBlock);

        llvm::PHINode *PN = llvm::PHINode::Create(CGF.BoolTy, 2, "", ContBlock);
        for (llvm::pred_iterator PI = pred_begin(ContBlock),
                                 PE = pred_end(ContBlock);
             PI != PE; ++PI)
          PN->addIncoming(Builder.getFalse(), *PI);

        Builder.SetInsertPoint(RHSBlock);
        llvm::Value *RHSCond =
            CGF.emitBoolExpr(BO->getRHS()).load(Builder, CGM);
        Builder.CreateBr(ContBlock);
        Builder.SetInsertPoint(ContBlock);
        PN->addIncoming(RHSCond, RHSBlock);

        V = PN;
      } else if (BO->getOpcode() == BO_LOr) {
        llvm::BasicBlock *ContBlock = CGF.createBasicBlock("lor.end");
        llvm::BasicBlock *RHSBlock = CGF.createBasicBlock("lor.rhs");
        CGF.emitBranchOnBoolExpr(BO->getLHS(), ContBlock, RHSBlock);

        llvm::PHINode *PN = llvm::PHINode::Create(CGF.BoolTy, 2, "", ContBlock);
        for (llvm::pred_iterator PI = pred_begin(ContBlock),
                                 PE = pred_end(ContBlock);
             PI != PE; ++PI)
          PN->addIncoming(Builder.getTrue(), *PI);

        Builder.SetInsertPoint(RHSBlock);
        llvm::Value *RHSCond =
            CGF.emitBoolExpr(BO->getRHS()).load(Builder, CGM);
        Builder.CreateBr(ContBlock);
        Builder.SetInsertPoint(ContBlock);
        PN->addIncoming(RHSCond, RHSBlock);

        V = PN;
      } else if (BO->getOpcode() == BO_LXor) {
        // TODO: implement LXor.
        assert(false && "logical xor not implemented");
        __builtin_unreachable();
      } else {
        assert(false && "unknown logical op");
        __builtin_unreachable();
      }
    }
    return ExprValue::getRValue(BO, V);
  }

  ExprValue visit(const CastExpr *CE) {
    ExprValue InVal = visit(CE->getSubExpr());
    if (CE->getCastKind() == CastKind::None) {
      return InVal;
    }

    const Type *OrigInTy = CE->getSubExpr()->getType().get();
    const Type *OrigOutTy = CE->getType().get();
    llvm::Value *In = InVal.load(Builder, CGM);

    switch (CE->getCastKind()) {
    case CastKind::None:
      assert(false);
      __builtin_unreachable();
    case CastKind::LValueToRValue: {
      return ExprValue::getRValue(CE, In);
    }
    case CastKind::IntegralCast: {
      auto OutTy = dynamic_cast<const IntegerType *>(OrigOutTy);
      auto InTy = dynamic_cast<const IntegerType *>(OrigInTy);
      assert(InTy != nullptr && OutTy != nullptr &&
             "IntegralCast should have int");
      llvm::Type *OutLLVMTy = CGM.getLLVMType(OutTy);
      llvm::Value *Out;
      if (InTy->isSigned()) {
        Out = Builder.CreateSExtOrTrunc(In, OutLLVMTy);
      } else {
        Out = Builder.CreateZExtOrTrunc(In, OutLLVMTy);
      }
      return ExprValue::getRValue(CE, Out);
    }
    case CastKind::TypeCast: {
      if (OrigInTy->getCategory() == OrigOutTy->getCategory()) {
        return ExprValue::getRValue(CE, In);
      }
      if (dynamic_cast<const AddressType *>(OrigInTy) ||
          dynamic_cast<const AddressType *>(OrigOutTy)) {
        return ExprValue::getRValue(
            CE, Builder.CreateZExtOrTrunc(
                    In, Builder.getIntNTy(OrigOutTy->getBitNum())));
      }
      if (dynamic_cast<const BooleanType *>(OrigInTy)) {
        llvm::Value *Out = Builder.CreateZExt(In, CGM.getLLVMType(OrigOutTy));
        return ExprValue::getRValue(CE, Out);
      }
      if (dynamic_cast<const BooleanType *>(OrigOutTy)) {
        llvm::Value *Out = Builder.CreateICmpNE(
            In, llvm::ConstantInt::getNullValue(In->getType()));
        return ExprValue::getRValue(CE, Out);
      }
      if ((dynamic_cast<const StringType *>(OrigInTy) &&
           dynamic_cast<const BytesType *>(OrigOutTy)) ||
          (dynamic_cast<const BytesType *>(OrigInTy) &&
           dynamic_cast<const StringType *>(OrigOutTy))) {
        llvm::Value *Out =
            llvm::ConstantAggregateZero::get(CGM.getLLVMType(OrigOutTy));
        Out = Builder.CreateInsertValue(
            Out, Builder.CreateExtractValue(In, {0}), {0});
        Out = Builder.CreateInsertValue(
            Out, Builder.CreateExtractValue(In, {1}), {1});
        return ExprValue::getRValue(CE, Out);
      }
      if (dynamic_cast<const IntegerType *>(OrigOutTy) &&
          dynamic_cast<const StringType *>(OrigInTy)) {
        // XXX: it's should be allowed in assembly only
        llvm::Value *Dst =
            Builder.CreateAlloca(CGM.Int8Ty, Builder.getInt16(32));
        llvm::Value *Length = Builder.CreateExtractValue(In, {0});
        llvm::Value *Src = Builder.CreateExtractValue(In, {1});
        CGM.emitMemcpy(Dst, Src,
                       Builder.CreateZExtOrTrunc(Length, CGM.Int32Ty));
        llvm::Value *Ptr = Builder.CreateBitCast(Dst, CGM.Int256PtrTy);
        return ExprValue::getRValue(
            CE, CGM.getEndianlessValue(Builder.CreateLoad(Ptr)));
      }
      assert(false);
      break;
    }
    }
    return ExprValue::getRValue(CE, nullptr);
  }

  ExprValue visit(const ParenExpr *PE) { return visit(PE->getSubExpr()); }

  ExprValue visit(const Identifier *ID) {
    if (ID->isSpecialIdentifier()) {
      switch (ID->getSpecialIdentifier()) {
      case Identifier::SpecialIdentifier::this_: {
        llvm::Value *Val = CGM.getEndianlessValue(CGM.emitGetAddress());
        return ExprValue::getRValue(ID, Val);
      }
      default:
        assert(false && "only SpecialIdentifier::this can change to address");
        __builtin_unreachable();
      }
    }
    const Decl *D = ID->getCorrespondDecl();

    if (auto *VD = dynamic_cast<const VarDecl *>(D)) {
      const Type *Ty = VD->GetType().get();
      if (VD->isStateVariable()) {
        return ExprValue(Ty, ValueKind::VK_SValue, CGM.getStateVarAddr(VD));
      } else {
        return ExprValue(Ty, ValueKind::VK_LValue, CGF.getAddrOfLocalVar(VD));
      }
    }
    assert(false && "unknown Identifier");
    __builtin_unreachable();
  }

  ExprValue visit(const CallExpr *CE) { return CGF.emitCallExpr(CE); }

  void emitCheckArrayOutOfBound(llvm::Value *ArrSz, llvm::Value *Index) {
    using namespace std::string_literals;
    static const std::string Message = "Array out of bound"s;
    llvm::Value *MessageValue =
        createGlobalStringPtr(CGF.getLLVMContext(), CGM.getModule(), Message);

    llvm::BasicBlock *Continue = CGF.createBasicBlock("continue");
    llvm::BasicBlock *Revert = CGF.createBasicBlock("revert");

    llvm::Value *OutOfBound = Builder.CreateICmpUGE(
        Index, Builder.CreateZExt(ArrSz, Index->getType()));
    Builder.CreateCondBr(OutOfBound, Revert, Continue);

    Builder.SetInsertPoint(Revert);
    CGM.emitRevert(MessageValue, Builder.getInt32(Message.size()));
    Builder.CreateUnreachable();

    Builder.SetInsertPoint(Continue);
  }

  ExprValue visit(const IndexAccess *IA) {
    ExprValue Base = visit(IA->getBase());
    ExprValue Index = visit(IA->getIndex());
    const Type *Ty = IA->getType().get();

    if (const auto *MType = dynamic_cast<const MappingType *>(Base.getType())) {
      llvm::Value *Pos =
          CGM.getEndianlessValue(Builder.CreateLoad(Base.getValue()));
      llvm::Value *Key;
      if (MType->getKeyType()->isDynamic()) {
        Key = Index.load(Builder, CGM);
      } else {
        Key = CGM.getEndianlessValue(
            Builder.CreateZExtOrTrunc(Index.load(Builder, CGM), CGF.Int256Ty));
      }
      llvm::Value *Bytes = CGM.emitConcateBytes({Key, Pos});
      llvm::Value *AddressPtr = Builder.CreateAlloca(CGF.Int256Ty);
      Builder.CreateStore(CGM.emitKeccak256(Bytes), AddressPtr);
      return ExprValue(Ty, ValueKind::VK_SValue, AddressPtr);
    }
    if (const auto *ArrTy = dynamic_cast<const ArrayType *>(Base.getType())) {
      // Array Type : Fixed Size Mem Array, Fixed Sized Storage Array, Dynamic
      // Sized Storage Array
      // Require Index to be unsigned 256-bit Int

      llvm::Value *IndexValue = Index.load(Builder, CGM);
      if (!IA->isStateVariable()) {
        // TODO : Assume only Integer Array
        llvm::Value *ArraySize = Builder.getInt(ArrTy->getLength());
        emitCheckArrayOutOfBound(ArraySize, IndexValue);
        llvm::Value *Address =
            Builder.CreateInBoundsGEP(CGM.getLLVMType(ArrTy), Base.getValue(),
                                      {Builder.getIntN(256, 0), IndexValue});
        return ExprValue(Ty, ValueKind::VK_LValue, Address);
      }

      llvm::Value *Pos = Builder.CreateLoad(Base.getValue());
      if (ArrTy->isDynamicSized()) {
        // load array size and check
        auto LengthTy = IntegerType::getIntN(256);
        ExprValue BaseLength(&LengthTy, ValueKind::VK_SValue, Base.getValue());
        llvm::Value *ArraySize = BaseLength.load(Builder, CGM);
        emitCheckArrayOutOfBound(ArraySize, IndexValue);

        // load array position
        llvm::Value *Bytes =
            CGM.emitConcateBytes({CGM.getEndianlessValue(Pos)});
        Pos = CGM.emitKeccak256(Bytes);
      } else {
        // Fixed Size Storage Array
        llvm::Value *ArraySize = Builder.getInt(ArrTy->getLength());
        emitCheckArrayOutOfBound(ArraySize, IndexValue);
      }

      unsigned BitPerElement = ArrTy->getElementType()->getBitNum();
      unsigned ElementPerSlot = 256 / BitPerElement;

      if (ElementPerSlot != 1) {
        llvm::Value *StorageIndex = Builder.CreateUDiv(
            IndexValue, Builder.getIntN(256, ElementPerSlot));
        llvm::Value *Shift = Builder.CreateURem(
            IndexValue, Builder.getIntN(256, ElementPerSlot));
        llvm::Value *ElemAddress = Builder.CreateAdd(Pos, StorageIndex);
        llvm::Value *Address = Builder.CreateAlloca(CGF.Int256Ty);
        Builder.CreateStore(ElemAddress, Address);
        return ExprValue(Ty, ValueKind::VK_SValue, Address, Shift);
      } else {
        llvm::Value *ElemAddress = Builder.CreateAdd(Pos, IndexValue);
        llvm::Value *Address = Builder.CreateAlloca(CGF.Int256Ty);
        Builder.CreateStore(ElemAddress, Address);
        return ExprValue(Ty, ValueKind::VK_SValue, Address);
      }
    }
    assert(false && "unknown IndexAccess type");
    __builtin_unreachable();
  }

  ExprValue visit(const MemberExpr *ME) {
    if (ME->getName()->isSpecialIdentifier()) {
      switch (ME->getName()->getSpecialIdentifier()) {
      case Identifier::SpecialIdentifier::msg_sender: {
        llvm::Value *Val = CGM.getEndianlessValue(CGM.emitGetCaller());
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::msg_value: {
        llvm::Value *Val = Builder.CreateZExt(
            CGM.getEndianlessValue(CGM.emitGetCallValue()), CGF.Int256Ty);
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::msg_data: {
        llvm::Value *CallDataSize = CGM.emitGetCallDataSize();
        llvm::Value *ValPtr = Builder.CreateAlloca(CGF.Int8Ty, CallDataSize);
        CGM.emitCallDataCopy(ValPtr, Builder.getInt32(0), CallDataSize);

        llvm::Value *Bytes = llvm::ConstantAggregateZero::get(CGF.BytesTy);
        Bytes = Builder.CreateInsertValue(
            Bytes, Builder.CreateZExt(CallDataSize, CGF.Int256Ty), {0});
        Bytes = Builder.CreateInsertValue(Bytes, ValPtr, {1});
        return ExprValue::getRValue(ME, Bytes);
      }
      case Identifier::SpecialIdentifier::msg_sig: {
        llvm::Value *ValPtr = Builder.CreateAlloca(Builder.getInt32Ty());
        CGM.emitCallDataCopy(Builder.CreateBitCast(ValPtr, CGF.Int8PtrTy),
                             Builder.getInt32(0), Builder.getInt32(4));
        llvm::Value *Val = Builder.CreateLoad(ValPtr);
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::tx_gasprice: {
        llvm::Value *Val = Builder.CreateZExt(
            CGM.getEndianlessValue(CGM.emitGetTxGasPrice()), CGF.Int256Ty);
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::tx_origin: {
        llvm::Value *Val = CGM.getEndianlessValue(CGM.emitGetTxOrigin());
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::block_coinbase: {
        llvm::Value *Val = CGM.getEndianlessValue(CGM.emitGetBlockCoinbase());
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::block_difficulty: {
        llvm::Value *Val = CGM.getEndianlessValue(CGM.emitGetBlockDifficulty());
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::block_gaslimit: {
        llvm::Value *Val = CGM.emitGetBlockGasLimit();
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::block_number: {
        llvm::Value *Val = CGM.emitGetBlockNumber();
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::now:
      case Identifier::SpecialIdentifier::block_timestamp: {
        llvm::Value *Val = CGM.emitGetBlockTimestamp();
        return ExprValue::getRValue(ME, Val);
      }
      case Identifier::SpecialIdentifier::address_balance: {
        llvm::Value *Address = CGF.emitExpr(ME->getBase()).load(Builder, CGM);
        llvm::Value *Val = CGM.getEndianlessValue(
            CGM.emitGetExternalBalance(CGM.getEndianlessValue(Address)));
        return ExprValue::getRValue(ME, Builder.CreateZExt(Val, CGF.Int256Ty));
      }
      default:
        assert(false && "unsupported special member access");
        __builtin_unreachable();
      }
    } else {
      ExprValue StructValue = visit(ME->getBase());
      auto ST =
          dynamic_cast<const StructType *>(ME->getBase()->getType().get());
      unsigned ElementIndex =
          ST->getElementIndex(ME->getName()->getName().str());
      if (StructValue.getValueKind() == ValueKind::VK_SValue) {
        llvm::Value *Base = Builder.CreateLoad(StructValue.getValue());
        llvm::Value *Pos =
            Builder.getIntN(256, ST->getStoragePos(ElementIndex));
        llvm::Value *ElemAddress = Builder.CreateAdd(Base, Pos);
        llvm::Value *Address = Builder.CreateAlloca(CGF.Int256Ty);
        Builder.CreateStore(ElemAddress, Address);
        return ExprValue(ME->getName()->getType().get(), ValueKind::VK_SValue,
                         Address);
      } else {
        llvm::Value *Value = StructValue.load(Builder, CGM);
        llvm::Value *Element =
            Builder.CreateExtractValue(Value, {ElementIndex});
        return ExprValue(ME->getName()->getType().get(), ValueKind::VK_LValue,
                         Element);
      }
    }

    assert(false && "unsupported non-special member access");
    __builtin_unreachable();
  }

  ExprValue visit(const BooleanLiteral *BL) {
    return ExprValue::getRValue(BL, Builder.getInt1(BL->getValue()));
  }

  ExprValue visit(const StringLiteral *SL) {
    const std::string &StringData = SL->getValue();
    llvm::Value *String = llvm::ConstantAggregateZero::get(CGF.StringTy);
    llvm::Constant *Ptr = createGlobalStringPtr(CGF.getLLVMContext(),
                                                CGM.getModule(), StringData);
    String = Builder.CreateInsertValue(
        String, Builder.getIntN(256, StringData.size()), {0});
    String = Builder.CreateInsertValue(String, Ptr, {1});

    return ExprValue::getRValue(SL, String);
  }

  ExprValue visit(const NumberLiteral *NL) {
    return ExprValue::getRValue(NL, Builder.getInt(NL->getValue()));
  }

  ExprValue visit(const AsmIdentifier *YI) {
    const Decl *D = YI->getCorrespondDecl();

    if (auto *VD = dynamic_cast<const AsmVarDecl *>(D)) {
      return ExprValue(VD->GetType().get(), ValueKind::VK_LValue,
                       CGF.getAddrOfLocalVar(VD));
    }
    __builtin_unreachable();
  }

  const Identifier *resolveIdentifier(const Expr *E) {
    if (auto Id = dynamic_cast<const Identifier *>(E)) {
      return Id;
    }
    return nullptr;
  }
};

ExprValue CodeGenFunction::emitExpr(const Expr *E) {
  return ExprEmitter(*this).visit(E);
}

void CodeGenFunction::emitCallRequire(const CallExpr *CE) {
  // require function
  auto Arguments = CE->getArguments();
  assert(Arguments.size() == 1 || Arguments.size() == 2);

  llvm::BasicBlock *Continue = createBasicBlock("continue");
  llvm::BasicBlock *Revert = createBasicBlock("revert");

  emitBranchOnBoolExpr(Arguments[0], Continue, Revert);

  Builder.SetInsertPoint(Revert);
  if (Arguments.size() == 2) {
    llvm::Value *Message = emitExpr(Arguments[1]).load(Builder, CGM);
    CGM.emitRevert(
        Builder.CreateExtractValue(Message, {1}),
        Builder.CreateTrunc(Builder.CreateExtractValue(Message, {0}), Int32Ty));
  }
  Builder.CreateUnreachable();

  Builder.SetInsertPoint(Continue);
}

void CodeGenFunction::emitCallAssert(const CallExpr *CE) {
  // assert function
  auto Arguments = CE->getArguments();
  assert(Arguments.size() == 1);

  llvm::BasicBlock *Continue = createBasicBlock("continue");
  llvm::BasicBlock *Revert = createBasicBlock("revert");

  emitBranchOnBoolExpr(Arguments[0], Continue, Revert);

  Builder.SetInsertPoint(Revert);
  Builder.CreateUnreachable();

  Builder.SetInsertPoint(Continue);
}

void CodeGenFunction::emitCallRevert(const CallExpr *CE) {
  // revert function
  auto Arguments = CE->getArguments();
  assert(Arguments.size() == 0 || Arguments.size() == 1);

  if (Arguments.size() == 1) {
    llvm::Value *Message = emitExpr(Arguments[0]).load(Builder, CGM);
    CGM.emitRevert(
        Builder.CreateExtractValue(Message, {1}),
        Builder.CreateTrunc(Builder.CreateExtractValue(Message, {0}), Int32Ty));
  }
  Builder.CreateUnreachable();
}

llvm::Value *CodeGenFunction::emitCallAddmod(const CallExpr *CE) {
  // addmod
  auto Arguments = CE->getArguments();
  llvm::Value *A = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *B = emitExpr(Arguments[1]).load(Builder, CGM);
  llvm::Value *K = emitExpr(Arguments[2]).load(Builder, CGM);
  llvm::Value *AddAB = Builder.CreateAdd(A, B);
  llvm::Value *Res = Builder.CreateURem(AddAB, K);
  return Res;
}

llvm::Value *CodeGenFunction::emitCallMulmod(const CallExpr *CE) {
  // mulmod
  auto Arguments = CE->getArguments();
  llvm::Value *A = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *B = emitExpr(Arguments[1]).load(Builder, CGM);
  llvm::Value *K = emitExpr(Arguments[2]).load(Builder, CGM);
  llvm::Value *MulAB = Builder.CreateMul(A, B);
  llvm::Value *Res = Builder.CreateURem(MulAB, K);
  return Res;
}

llvm::Value *CodeGenFunction::emitCallKeccak256(const CallExpr *CE) {
  // keccak256 function
  auto Arguments = CE->getArguments();
  llvm::Value *MessageValue = emitExpr(Arguments[0]).load(Builder, CGM);
  return CGM.emitKeccak256(MessageValue);
}

llvm::Value *CodeGenFunction::emitCallSha256(const CallExpr *CE) {
  // sha256 function
  auto Arguments = CE->getArguments();
  llvm::Value *MessageValue = emitExpr(Arguments[0]).load(Builder, CGM);
  return CGM.emitSha256(MessageValue);
}

llvm::Value *CodeGenFunction::emitCallRipemd160(const CallExpr *CE) {
  // ripemd160 function
  auto Arguments = CE->getArguments();
  llvm::Value *MessageValue = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Val = Builder.CreateCall(
      CGM.getModule().getFunction("solidity.ripemd160"), {MessageValue});
  return Builder.CreateTrunc(Val, Int160Ty);
}

llvm::Value *CodeGenFunction::emitCallEcrecover(const CallExpr *CE) {
  // ecrecover function
  auto Arguments = CE->getArguments();
  llvm::Value *HashValue = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *RecoveryIdValue =
      Builder.CreateZExt(emitExpr(Arguments[1]).load(Builder, CGM), Int256Ty);
  llvm::Value *RValue = emitExpr(Arguments[2]).load(Builder, CGM);
  llvm::Value *SValue = emitExpr(Arguments[3]).load(Builder, CGM);
  llvm::Value *Val =
      Builder.CreateCall(CGM.getModule().getFunction("solidity.ecrecover"),
                         {HashValue, RecoveryIdValue, RValue, SValue});
  return Builder.CreateTrunc(Val, AddressTy);
}

llvm::Value *CodeGenFunction::emitCallBlockHash(const CallExpr *CE) {
  // blockhash function
  auto Arguments = CE->getArguments();
  llvm::Value *NumberValue = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Val = CGM.getEndianlessValue(CGM.emitGetBlockHash(NumberValue));
  return Val;
}

llvm::Value *CodeGenFunction::emitCallAddressStaticcall(const CallExpr *CE,
                                                        const MemberExpr *ME) {
  // address_staticcall function
  auto Arguments = CE->getArguments();
  llvm::Value *Address = emitExpr(ME->getBase()).load(Builder, CGM);
  llvm::Value *AddressPtr = Builder.CreateAlloca(AddressTy);
  Builder.CreateStore(CGM.getEndianlessValue(Address), AddressPtr);
  llvm::Value *MemoryValue = emitExpr(Arguments[0]).load(Builder, CGM);

  llvm::Value *Length = Builder.CreateTrunc(
      Builder.CreateExtractValue(MemoryValue, {0}), Int32Ty, "length");
  llvm::Value *Ptr = Builder.CreateExtractValue(MemoryValue, {1}, "ptr");
  llvm::Value *Gas = CGM.emitGetGasLeft();

  llvm::Value *Val = CGM.emitCallStatic(Gas, AddressPtr, Ptr, Length);

  using namespace std::string_literals;
  static const std::string Message = "StaticCall Fail"s;
  llvm::Constant *MessageValue =
      createGlobalStringPtr(getLLVMContext(), CGM.getModule(), Message);

  llvm::Value *Cond = Builder.CreateICmpNE(Val, Builder.getInt32(2));
  llvm::BasicBlock *Continue = createBasicBlock("continue");
  llvm::BasicBlock *Revert = createBasicBlock("revert");
  Builder.CreateCondBr(Cond, Continue, Revert);

  Builder.SetInsertPoint(Revert);
  CGM.emitRevert(MessageValue, Builder.getInt32(Message.size()));
  Builder.CreateUnreachable();

  Builder.SetInsertPoint(Continue);
  llvm::Value *ReturnDataSize = CGM.emitReturnDataSize();
  llvm::Value *ReturnData =
      CGM.emitReturnDataCopyBytes(Builder.getInt32(0), ReturnDataSize);
  (void)ReturnData; // -Wunused-variable
  // Todo:
  // return tuple (~bool(Val), ReturnData)
  return Builder.CreateNot(Builder.CreateTrunc(Val, BoolTy));
}

llvm::Value *
CodeGenFunction::emitCallAddressDelegatecall(const CallExpr *CE,
                                             const MemberExpr *ME) {
  // address_staticcall function
  auto Arguments = CE->getArguments();
  llvm::Value *Address = emitExpr(ME->getBase()).load(Builder, CGM);
  llvm::Value *AddressPtr = Builder.CreateAlloca(AddressTy);
  Builder.CreateStore(CGM.getEndianlessValue(Address), AddressPtr);
  llvm::Value *MemoryValue = emitExpr(Arguments[0]).load(Builder, CGM);

  llvm::Value *Length = Builder.CreateTrunc(
      Builder.CreateExtractValue(MemoryValue, {0}), Int32Ty, "length");
  llvm::Value *Ptr = Builder.CreateExtractValue(MemoryValue, {1}, "ptr");
  llvm::Value *Gas = CGM.emitGetGasLeft();

  llvm::Value *Val = CGM.emitCallDelegate(Gas, AddressPtr, Ptr, Length);

  llvm::Value *Cond;
  if (CGM.isEWASM()) {
    Cond = Builder.CreateICmpEQ(Val, Builder.getInt32(0));
  } else if (CGM.isEVM()) {
    Cond = Builder.CreateICmpEQ(Val, Builder.getInt32(1));
  } else {
    __builtin_unreachable();
  }

  llvm::Value *ReturnDataSize = CGM.emitReturnDataSize();
  llvm::Value *ReturnData =
      CGM.emitReturnDataCopyBytes(Builder.getInt32(0), ReturnDataSize);
  (void)ReturnData; // -Wunused-variable
  // Todo:
  // return tuple (Cond, ReturnData)
  return Builder.CreateTrunc(Cond, BoolTy);
}

llvm::Value *CodeGenFunction::emitCallAddressCall(const CallExpr *CE,
                                                  const MemberExpr *ME) {
  // address_call function
  auto Arguments = CE->getArguments();
  llvm::Value *Address = emitExpr(ME->getBase()).load(Builder, CGM);
  llvm::Value *AddressPtr = Builder.CreateAlloca(AddressTy);
  Builder.CreateStore(CGM.getEndianlessValue(Address), AddressPtr);
  llvm::Value *MemoryValue = emitExpr(Arguments[0]).load(Builder, CGM);

  llvm::Value *Length = Builder.CreateTrunc(
      Builder.CreateExtractValue(MemoryValue, {0}), Int32Ty, "length");
  llvm::Value *Ptr = Builder.CreateExtractValue(MemoryValue, {1}, "ptr");
  llvm::Value *Gas = CGM.emitGetGasLeft();
  llvm::Value *ValuePtr = Builder.CreateAlloca(Int128Ty);
  Builder.CreateStore(Builder.getIntN(128, 0), ValuePtr);

  llvm::Value *Val = CGM.emitCall(Gas, AddressPtr, ValuePtr, Ptr, Length);

  llvm::Value *Cond;
  if (CGM.isEWASM()) {
    Cond = Builder.CreateICmpEQ(Val, Builder.getInt32(0));
  } else if (CGM.isEVM()) {
    Cond = Builder.CreateICmpEQ(Val, Builder.getInt32(1));
  } else {
    __builtin_unreachable();
  }

  llvm::Value *ReturnDataSize = CGM.emitReturnDataSize();
  llvm::Value *ReturnData =
      CGM.emitReturnDataCopyBytes(Builder.getInt32(0), ReturnDataSize);
  (void)ReturnData; // -Wunused-variable
  // Todo:
  // return tuple (Cond, ReturnData)
  return Builder.CreateTrunc(Cond, BoolTy);
}

llvm::Value *CodeGenFunction::emitCallAddressSend(const CallExpr *CE,
                                                  const MemberExpr *ME,
                                                  bool NeedRevert) {
  // address_call function
  auto Arguments = CE->getArguments();
  llvm::Value *Address = emitExpr(ME->getBase()).load(Builder, CGM);
  llvm::Value *AddressPtr = Builder.CreateAlloca(AddressTy);
  Builder.CreateStore(CGM.getEndianlessValue(Address), AddressPtr);
  llvm::Value *Amount = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *AmountPtr = Builder.CreateAlloca(Int128Ty);
  Builder.CreateStore(
      CGM.getEndianlessValue(Builder.CreateZExtOrTrunc(Amount, Int128Ty)),
      AmountPtr);

  llvm::Value *Length = Builder.getInt32(0);
  llvm::Value *Ptr = Builder.CreateAlloca(Int8Ty, Length);
  llvm::Value *Gas = Builder.getInt64(0);

  llvm::Value *Val = CGM.emitCall(Gas, AddressPtr, AmountPtr, Ptr, Length);

  llvm::Value *Cond;
  if (CGM.isEWASM()) {
    Cond = Builder.CreateICmpEQ(Val, Builder.getInt32(0));
  } else if (CGM.isEVM()) {
    Cond = Builder.CreateICmpEQ(Val, Builder.getInt32(1));
  } else {
    __builtin_unreachable();
  }

  if (!NeedRevert) {
    return Builder.CreateTrunc(Cond, BoolTy);
  }

  using namespace std::string_literals;
  static const std::string Message = "Transfer Fail"s;
  llvm::Constant *MessageValue =
      createGlobalStringPtr(getLLVMContext(), CGM.getModule(), Message);

  llvm::BasicBlock *Continue = createBasicBlock("continue");
  llvm::BasicBlock *Revert = createBasicBlock("revert");
  Builder.CreateCondBr(Cond, Continue, Revert);

  Builder.SetInsertPoint(Revert);
  CGM.emitRevert(MessageValue, Builder.getInt32(Message.size()));
  Builder.CreateUnreachable();
  Builder.SetInsertPoint(Continue);
  return nullptr;
}

llvm::Value *CodeGenFunction::emitAbiEncodePacked(const CallExpr *CE) {
  // abi_encodePacked
  // TODO: we need to support array, tuple, struct
  auto Arguments = CE->getArguments();
  std::vector<llvm::Value *> Args;
  for (const auto &Arg : Arguments) {
    llvm::Value *Value = emitExpr(Arg).load(Builder, CGM);
    if (Arg->getType()->getCategory() == Type::Category::Bool) {
      Value = Builder.CreateZExt(Value, Int8Ty);
    }
    if (Arg->getType()->shouldEndianLess()) {
      Value = CGM.getEndianlessValue(Value);
    }
    Args.emplace_back(Value);
  }
  llvm::Value *Bytes =
      CGM.emitConcateBytes(llvm::ArrayRef<llvm::Value *>(Args));
  return Bytes;
}

llvm::Value *CodeGenFunction::emitAbiEncode(const CallExpr *CE) {
  // abi_encode
  // TODO: we need to support array, tuple, struct
  auto Arguments = CE->getArguments();
  std::vector<llvm::Value *> Head;
  std::vector<std::pair<llvm::Value *, size_t>> Tail;
  unsigned Pos = 0;
  for (const auto &Arg : Arguments) {
    llvm::Value *Value = emitExpr(Arg).load(Builder, CGM);
    if (Arg->getType()->isDynamic()) {
      Tail.emplace_back(Value, Head.size());
      Head.emplace_back(nullptr);
    } else {
      if (Arg->getType()->shouldEndianLess()) {
        Head.emplace_back(
            CGM.getEndianlessValue(Builder.CreateZExt(Value, Int256Ty)));
      } else {
        Head.emplace_back(Builder.CreateZExt(Value, Int256Ty));
      }
    }
    Pos += Arg->getType()->getABIStaticSize();
  }
  llvm::Value *TailPos = Builder.getIntN(256, Pos);
  for (auto Arg : Tail) {
    llvm::Value *Value;
    size_t HeadPos;
    std::tie(Value, HeadPos) = Arg;
    Head[HeadPos] = CGM.getEndianlessValue(TailPos);
    llvm::Value *Length = Builder.CreateZExtOrTrunc(
        Builder.CreateExtractValue(Value, {0}), Int256Ty);
    Head.emplace_back(CGM.getEndianlessValue(Length));
    TailPos = Builder.CreateAdd(TailPos, Builder.getIntN(256, 32));
    Head.emplace_back(Value);
    llvm::Value *LowBits = Builder.getInt(llvm::APInt::getLowBitsSet(256, 5));
    llvm::Value *PadRightLength =
        Builder.CreateAnd(Builder.CreateSub(Builder.getIntN(256, 32),
                                            Builder.CreateAnd(Length, LowBits)),
                          LowBits);
    // TODO: use memory allocate
    llvm::Value *PadRightArray = Builder.CreateAlloca(Int8Ty, PadRightLength);
    llvm::Value *PadRightBytes = llvm::ConstantAggregateZero::get(BytesTy);
    PadRightBytes = Builder.CreateInsertValue(
        PadRightBytes, Builder.CreateZExtOrTrunc(PadRightLength, Int256Ty),
        {0});
    PadRightBytes =
        Builder.CreateInsertValue(PadRightBytes, PadRightArray, {1});
    Head.emplace_back(PadRightBytes);
    TailPos =
        Builder.CreateAdd(TailPos, Builder.CreateAdd(Length, PadRightLength));
  }
  llvm::Value *Bytes =
      CGM.emitConcateBytes(llvm::ArrayRef<llvm::Value *>(Head));
  return Bytes;
}

ExprValue CodeGenFunction::emitCallExpr(const CallExpr *CE) {
  auto Expr = CE->getCalleeExpr();
  auto ME = dynamic_cast<const MemberExpr *>(Expr);
  if (ME) {
    Expr = ME->getName();
  }
  const Decl *D = nullptr;
  if (auto *Callee = dynamic_cast<const Identifier *>(Expr)) {
    if (Callee->isSpecialIdentifier()) {
      return emitSpecialCallExpr(Callee, CE, ME);
    } else {
      D = Callee->getCorrespondDecl();
    }
  } else if (auto *Callee = dynamic_cast<const AsmIdentifier *>(Expr)) {
    if (Callee->isSpecialIdentifier()) {
      return emitAsmSpecialCallExpr(Callee, CE);
    } else {
      D = Callee->getCorrespondDecl();
    }
  } else {
    __builtin_unreachable();
  }
  std::vector<llvm::Value *> Args;
  for (auto Argument : CE->getArguments()) {
    Args.push_back(emitExpr(Argument).load(Builder, CGM));
  }
  if (auto FD = dynamic_cast<const FunctionDecl *>(D)) {
    llvm::Function *F = CGM.createOrGetLLVMFunction(FD);
    assert(F != nullptr && "undefined function");
    llvm::Value *Result = Builder.CreateCall(F, Args);
    return ExprValue::getRValue(CE, Result);
  }
  if (auto ED = dynamic_cast<const EventDecl *>(D)) {
    auto Params = ED->getParams()->getParams();
    auto Arguments = CE->getArguments();
    std::vector<llvm::Value *> Data(
        1, llvm::ConstantPointerNull::get(Int256PtrTy));
    std::vector<llvm::Value *> Topics;
    uint DataCnt = 0;

    if (ED->isAnonymous() == false) {
      Topics.emplace_back(
          Builder.CreateBitCast(CGM.getModule().getNamedGlobal(
                                    "solidity.event." + CGM.getMangledName(ED)),
                                Int256PtrTy));
    }

    // XXX: Multiple args and complex data type encoding not implemented yet.
    for (size_t I = 0; I < Arguments.size(); I++) {
      llvm::Value *ValPtr = Builder.CreateAlloca(Int256Ty, nullptr);
      Builder.CreateStore(
          CGM.getEndianlessValue(Builder.CreateZExtOrTrunc(Args[I], Int256Ty)),
          ValPtr);
      if (dynamic_cast<const VarDecl *>(Params[I])->isIndexed()) {
        Topics.emplace_back(ValPtr);
      } else {
        Data[DataCnt++] = ValPtr;
      }
    }
    CGM.emitLog(Data[0], Builder.getInt32(32), Topics);
    return ExprValue();
  }
  if (auto AFD = dynamic_cast<const AsmFunctionDecl *>(D)) {
    llvm::Function *F = CGM.createOrGetLLVMFunction(AFD);
    assert(F != nullptr && "undefined function");
    llvm::Value *Result = Builder.CreateCall(F, Args);
    return ExprValue::getRValue(CE, Result);
  }
  assert(false && "Unhandle CallExprType CodeGen case.");
  __builtin_unreachable();
}

ExprValue CodeGenFunction::emitSpecialCallExpr(const Identifier *SI,
                                               const CallExpr *CE,
                                               const MemberExpr *ME) {
  switch (SI->getSpecialIdentifier()) {
  case Identifier::SpecialIdentifier::require:
    emitCallRequire(CE);
    return ExprValue();
  case Identifier::SpecialIdentifier::assert_:
    emitCallAssert(CE);
    return ExprValue();
  case Identifier::SpecialIdentifier::revert:
    emitCallRevert(CE);
    return ExprValue();
  case Identifier::SpecialIdentifier::gasleft:
    assert(CE->getArguments().empty() && "gasleft require no arguments");
    return ExprValue::getRValue(CE, CGM.emitGetGasLeft());
  case Identifier::SpecialIdentifier::addmod:
    return ExprValue::getRValue(CE, emitCallAddmod(CE));
  case Identifier::SpecialIdentifier::mulmod:
    return ExprValue::getRValue(CE, emitCallMulmod(CE));
  case Identifier::SpecialIdentifier::keccak256:
    return ExprValue::getRValue(CE, emitCallKeccak256(CE));
  case Identifier::SpecialIdentifier::sha256:
    return ExprValue::getRValue(CE, emitCallSha256(CE));
  case Identifier::SpecialIdentifier::ripemd160:
    return ExprValue::getRValue(CE, emitCallRipemd160(CE));
  case Identifier::SpecialIdentifier::ecrecover:
    return ExprValue::getRValue(CE, emitCallEcrecover(CE));
  case Identifier::SpecialIdentifier::blockhash:
    return ExprValue::getRValue(CE, emitCallBlockHash(CE));
  case Identifier::SpecialIdentifier::address_staticcall:
    return ExprValue::getRValue(CE, emitCallAddressStaticcall(CE, ME));
  case Identifier::SpecialIdentifier::address_delegatecall:
    return ExprValue::getRValue(CE, emitCallAddressDelegatecall(CE, ME));
  case Identifier::SpecialIdentifier::address_call:
    return ExprValue::getRValue(CE, emitCallAddressCall(CE, ME));
  case Identifier::SpecialIdentifier::address_transfer:
    emitCallAddressSend(CE, ME, true);
    return ExprValue();
  case Identifier::SpecialIdentifier::address_send:
    return ExprValue::getRValue(CE, emitCallAddressSend(CE, ME, false));
  case Identifier::SpecialIdentifier::abi_encodePacked:
    return ExprValue::getRValue(CE, emitAbiEncodePacked(CE));
  case Identifier::SpecialIdentifier::abi_encode:
    return ExprValue::getRValue(CE, emitAbiEncode(CE));
  default:
    assert(false && "special function not supported yet");
    __builtin_unreachable();
  }
}

llvm::Value *CodeGenFunction::emitAsmCallDataSize(const CallExpr *CE) {
  if (auto *ICE =
          dynamic_cast<const ImplicitCastExpr *>(CE->getArguments()[0])) {
    if (auto *SL = dynamic_cast<const StringLiteral *>(ICE->getSubExpr())) {
      std::string Name = SL->getValue();
      auto ObjectOrData = CGM.lookupYulDataOrYulObject(Name);
      return std::visit(
          [this, &Name](auto Ptr) -> llvm::Value * {
            if constexpr (std::is_same_v<decltype(Ptr), const YulData *>) {
              llvm::GlobalVariable *Data = CGM.getYulDataAddr(Ptr);
              auto Size = Data->getType()
                              ->getPointerElementType()
                              ->getArrayNumElements();
              return Builder.getIntN(256, Size);
            } else if constexpr (std::is_same_v<decltype(Ptr),
                                                const YulObject *>) {
              llvm::Function *Object =
                  CGM.getModule().getFunction(Name + ".object");
              auto *ObjectBytes = Builder.CreateCall(Object);
              return Builder.CreateExtractValue(ObjectBytes, {0});
            } else {
              assert(false && "datasize needs a valid data name");
              return Builder.getIntN(256, 0);
            }
          },
          ObjectOrData);
    }
  }
  assert(false && "dataoffset() needs a string literal");
  __builtin_unreachable();
}

llvm::Value *CodeGenFunction::emitAsmCallDataOffset(const CallExpr *CE) {
  if (auto *ICE =
          dynamic_cast<const ImplicitCastExpr *>(CE->getArguments()[0])) {
    if (auto *SL = dynamic_cast<const StringLiteral *>(ICE->getSubExpr())) {
      std::string Name = SL->getValue();
      auto ObjectOrData = CGM.lookupYulDataOrYulObject(Name);
      return std::visit(
          [this, &Name](auto Ptr) -> llvm::Value * {
            if constexpr (std::is_same_v<decltype(Ptr), const YulData *>) {
              llvm::GlobalVariable *Data = CGM.getYulDataAddr(Ptr);
              return Builder.CreatePtrToInt(Data, Int256Ty);
            } else if constexpr (std::is_same_v<decltype(Ptr),
                                                const YulObject *>) {
              llvm::Function *Object =
                  CGM.getModule().getFunction(Name + ".object");
              auto *ObjectBytes = Builder.CreateCall(Object);
              auto *Data = Builder.CreateExtractValue(ObjectBytes, {1});
              return Builder.CreatePtrToInt(Data, Int256Ty);
            } else {
              assert(false && "dataoffset needs a valid data name");
              return Builder.getIntN(256, 0);
            }
          },
          ObjectOrData);
    }
  }
  assert(false && "dataoffset() needs a string literal");
  __builtin_unreachable();
}

llvm::Value *CodeGenFunction::emitAsmCallMLoad(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *CPtr =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.cptr");
  llvm::Value *Ptr = Builder.CreateBitCast(CPtr, Int256PtrTy, "heap.ptr");
  llvm::Value *Value = CGM.getEndianlessValue(Builder.CreateLoad(Ptr));
  CGM.emitUpdateMemorySize(Pos, Builder.getIntN(256, 32));
  return Value;
}

void CodeGenFunction::emitAsmCallMStore(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *CPtr =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.cptr");
  llvm::Value *Ptr = Builder.CreateBitCast(CPtr, Int256PtrTy, "heap.ptr");
  Builder.CreateStore(
      CGM.getEndianlessValue(emitExpr(Arguments[1]).load(Builder, CGM)), Ptr);
  CGM.emitUpdateMemorySize(Pos, Builder.getIntN(256, 32));
}

void CodeGenFunction::emitAsmCallMStore8(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Ptr =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.ptr");
  llvm::Value *Value = Builder.CreateZExtOrTrunc(
      emitExpr(Arguments[1]).load(Builder, CGM), CGM.Int8Ty);
  Builder.CreateStore(Value, Ptr);
  CGM.emitUpdateMemorySize(Pos, Builder.getIntN(256, 1));
}

llvm::Value *CodeGenFunction::emitAsmCallMSize(const CallExpr *CE) {
  return Builder.CreateLoad(CGM.getMemorySize());
}

llvm::Value *CodeGenFunction::emitAsmCallSLoad(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  auto Val =
      CGM.emitStorageLoad(CGM.getEndianlessValue(Builder.CreateZExtOrTrunc(
          emitExpr(Arguments[0]).load(Builder, CGM), CGM.Int256Ty)));
  return CGM.getEndianlessValue(Val);
}

void CodeGenFunction::emitAsmCallSStore(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  CGM.emitStorageStore(
      CGM.getEndianlessValue(Builder.CreateZExtOrTrunc(
          emitExpr(Arguments[0]).load(Builder, CGM), CGM.Int256Ty)),
      CGM.getEndianlessValue(Builder.CreateZExtOrTrunc(
          emitExpr(Arguments[1]).load(Builder, CGM), CGM.Int256Ty)));
}

void CodeGenFunction::emitAsmCallReturn(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Ptr =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.cptr");
  llvm::Value *Length = emitExpr(Arguments[1]).load(Builder, CGM);
  CGM.emitUpdateMemorySize(Pos, Length);
  CGM.emitFinish(Ptr, Builder.CreateZExtOrTrunc(Length, CGM.Int32Ty));
}

void CodeGenFunction::emitAsmCallRevert(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Ptr =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.cptr");
  llvm::Value *Length = emitExpr(Arguments[1]).load(Builder, CGM);
  CGM.emitUpdateMemorySize(Pos, Length);
  CGM.emitRevert(Ptr, Builder.CreateZExtOrTrunc(Length, CGM.Int32Ty));
}

void CodeGenFunction::emitAsmCallLog(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Data =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.cptr");
  llvm::Value *DataLength = emitExpr(Arguments[1]).load(Builder, CGM);
  std::vector<llvm::Value *> Topics;
  for (size_t I = 2; I < Arguments.size(); I++) {
    llvm::Value *ValPtr = Builder.CreateAlloca(Int256Ty, nullptr);
    Builder.CreateStore(
        CGM.getEndianlessValue(Builder.CreateZExtOrTrunc(
            emitExpr(Arguments[I]).load(Builder, CGM), CGM.Int256Ty)),
        ValPtr);
    Topics.emplace_back(ValPtr);
  }
  CGM.emitUpdateMemorySize(Pos, DataLength);
  CGM.emitLog(Data, Builder.CreateZExtOrTrunc(DataLength, CGM.Int32Ty), Topics);
}

llvm::Value *CodeGenFunction::emitAsmCallCallDataLoad(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  return CGM.getEndianlessValue(
      CGM.emitCallDataLoad(emitExpr(Arguments[0]).load(Builder, CGM)));
}

void CodeGenFunction::emitAsmCallCodeCopy(const CallExpr *CE) {
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Ptr =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.ptr");
  llvm::Value *Code = Builder.CreateIntToPtr(
      emitExpr(Arguments[1]).load(Builder, CGM), Int8PtrTy);
  llvm::Value *Length = emitExpr(Arguments[2]).load(Builder, CGM);
  CGM.emitUpdateMemorySize(Pos, Length);
  CGM.emitMemcpy(Ptr, Code, Builder.CreateZExtOrTrunc(Length, CGM.Int32Ty));
}

llvm::Value *CodeGenFunction::emitAsmCallkeccak256(const CallExpr *CE) {
  // keccak256 function
  auto Arguments = CE->getArguments();
  llvm::Value *Pos = emitExpr(Arguments[0]).load(Builder, CGM);
  llvm::Value *Ptr =
      Builder.CreateInBoundsGEP(CGM.getHeapBase(), {Pos}, "heap.cptr");
  llvm::Value *Length = emitExpr(Arguments[1]).load(Builder, CGM);
  CGM.emitUpdateMemorySize(Pos, Length);
  return CGM.emitKeccak256(Ptr, Length);
}

ExprValue CodeGenFunction::emitAsmSpecialCallExpr(const AsmIdentifier *SI,
                                                  const CallExpr *CE) {
  switch (SI->getSpecialIdentifier()) {
  /// logical
  case AsmIdentifier::SpecialIdentifier::not_: ///< should be handled in Sema
  case AsmIdentifier::SpecialIdentifier::and_:
  case AsmIdentifier::SpecialIdentifier::or_:
  case AsmIdentifier::SpecialIdentifier::xor_:
  /// arithmetic
  case AsmIdentifier::SpecialIdentifier::addu256:
  case AsmIdentifier::SpecialIdentifier::subu256:
  case AsmIdentifier::SpecialIdentifier::mulu256:
  case AsmIdentifier::SpecialIdentifier::divu256:
  case AsmIdentifier::SpecialIdentifier::divs256:
  case AsmIdentifier::SpecialIdentifier::modu256:
  case AsmIdentifier::SpecialIdentifier::mods256:
  case AsmIdentifier::SpecialIdentifier::expu256:
  case AsmIdentifier::SpecialIdentifier::ltu256:
  case AsmIdentifier::SpecialIdentifier::gtu256:
  case AsmIdentifier::SpecialIdentifier::lts256:
  case AsmIdentifier::SpecialIdentifier::gts256:
  case AsmIdentifier::SpecialIdentifier::equ256:
  case AsmIdentifier::SpecialIdentifier::notu256:
  case AsmIdentifier::SpecialIdentifier::andu256:
  case AsmIdentifier::SpecialIdentifier::oru256:
  case AsmIdentifier::SpecialIdentifier::xoru256:
  case AsmIdentifier::SpecialIdentifier::shlu256:
  case AsmIdentifier::SpecialIdentifier::shru256:
    assert(false && "unexpected special identifiers");
    __builtin_unreachable();
  case AsmIdentifier::SpecialIdentifier::addmodu256:
    return ExprValue::getRValue(CE, emitCallAddmod(CE));
  case AsmIdentifier::SpecialIdentifier::mulmodu256:
    return ExprValue::getRValue(CE, emitCallMulmod(CE));
  /// memory and storage
  case AsmIdentifier::SpecialIdentifier::mload:
    return ExprValue::getRValue(CE, emitAsmCallMLoad(CE));
  case AsmIdentifier::SpecialIdentifier::mstore:
    emitAsmCallMStore(CE);
    return ExprValue();
  case AsmIdentifier::SpecialIdentifier::mstore8:
    emitAsmCallMStore8(CE);
    return ExprValue();
  case AsmIdentifier::SpecialIdentifier::msize:
    return ExprValue::getRValue(CE, emitAsmCallMSize(CE));
  case AsmIdentifier::SpecialIdentifier::sload:
    return ExprValue::getRValue(CE, emitAsmCallSLoad(CE));
  case AsmIdentifier::SpecialIdentifier::sstore:
    emitAsmCallSStore(CE);
    return ExprValue();
  /// execution control
  case AsmIdentifier::SpecialIdentifier::stop:
    CGM.emitFinish(llvm::ConstantPointerNull::get(Int8PtrTy),
                   Builder.getInt32(0));
    return ExprValue();
  case AsmIdentifier::SpecialIdentifier::return_:
    emitAsmCallReturn(CE);
    return ExprValue();
  case AsmIdentifier::SpecialIdentifier::revert:
    emitAsmCallRevert(CE);
    return ExprValue();
  case AsmIdentifier::SpecialIdentifier::log0:
  case AsmIdentifier::SpecialIdentifier::log1:
  case AsmIdentifier::SpecialIdentifier::log2:
  case AsmIdentifier::SpecialIdentifier::log3:
  case AsmIdentifier::SpecialIdentifier::log4:
    emitAsmCallLog(CE);
    return ExprValue();
  /// state
  case AsmIdentifier::SpecialIdentifier::blockcoinbase:
    return ExprValue::getRValue(
        CE,
        Builder.CreateZExtOrTrunc(
            CGM.getEndianlessValue(CGM.emitGetBlockCoinbase()), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::blockdifficulty:
    return ExprValue::getRValue(
        CE, CGM.getEndianlessValue(CGM.emitGetBlockDifficulty()));
  case AsmIdentifier::SpecialIdentifier::blockgaslimit:
    return ExprValue::getRValue(
        CE,
        Builder.CreateZExtOrTrunc(CGM.emitGetBlockGasLimit(), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::blocknumber:
    return ExprValue::getRValue(
        CE, Builder.CreateZExtOrTrunc(CGM.emitGetBlockNumber(), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::blocktimestamp:
    return ExprValue::getRValue(
        CE,
        Builder.CreateZExtOrTrunc(CGM.emitGetBlockTimestamp(), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::txorigin:
    return ExprValue::getRValue(
        CE, Builder.CreateZExtOrTrunc(
                CGM.getEndianlessValue(CGM.emitGetTxOrigin()), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::txgasprice:
    return ExprValue::getRValue(
        CE, Builder.CreateZExtOrTrunc(
                CGM.getEndianlessValue(CGM.emitGetTxGasPrice()), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::gasleft:
    return ExprValue::getRValue(
        CE, Builder.CreateZExtOrTrunc(CGM.emitGetGasLeft(), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::caller:
    return ExprValue::getRValue(
        CE, Builder.CreateZExtOrTrunc(
                CGM.getEndianlessValue(CGM.emitGetCaller()), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::callvalue:
    return ExprValue::getRValue(
        CE, Builder.CreateZExtOrTrunc(
                CGM.getEndianlessValue(CGM.emitGetCallValue()), CGM.Int256Ty));
  case AsmIdentifier::SpecialIdentifier::calldataload:
    return ExprValue::getRValue(CE, emitAsmCallCallDataLoad(CE));
  case AsmIdentifier::SpecialIdentifier::calldatasize:
    return ExprValue::getRValue(
        CE, Builder.CreateZExtOrTrunc(CGM.emitGetCallDataSize(), CGM.Int256Ty));
  /// object
  case AsmIdentifier::SpecialIdentifier::dataoffset:
    return ExprValue::getRValue(CE, emitAsmCallDataOffset(CE));
  case AsmIdentifier::SpecialIdentifier::datasize:
    return ExprValue::getRValue(CE, emitAsmCallDataSize(CE));
  case AsmIdentifier::SpecialIdentifier::datacopy:
  case AsmIdentifier::SpecialIdentifier::codecopy:
    emitAsmCallCodeCopy(CE);
    return ExprValue();
  /// misc
  case AsmIdentifier::SpecialIdentifier::keccak256:
    return ExprValue::getRValue(CE, emitAsmCallkeccak256(CE));
  // TODO:
  // - implement special identifiers below and
  //   move implemented ones above this TODO.
  case AsmIdentifier::SpecialIdentifier::signextendu256:
  case AsmIdentifier::SpecialIdentifier::iszerou256:
  case AsmIdentifier::SpecialIdentifier::sars256:
  case AsmIdentifier::SpecialIdentifier::byte:
  case AsmIdentifier::SpecialIdentifier::create:
  case AsmIdentifier::SpecialIdentifier::create2:
  case AsmIdentifier::SpecialIdentifier::call:
  case AsmIdentifier::SpecialIdentifier::callcode:
  case AsmIdentifier::SpecialIdentifier::delegatecall:
  case AsmIdentifier::SpecialIdentifier::abort:
  case AsmIdentifier::SpecialIdentifier::selfdestruct:
  case AsmIdentifier::SpecialIdentifier::blockhash:
  case AsmIdentifier::SpecialIdentifier::balance:
  case AsmIdentifier::SpecialIdentifier::this_:
  case AsmIdentifier::SpecialIdentifier::calldatacopy:
  case AsmIdentifier::SpecialIdentifier::codesize:
  case AsmIdentifier::SpecialIdentifier::extcodesize:
  case AsmIdentifier::SpecialIdentifier::extcodecopy:
  case AsmIdentifier::SpecialIdentifier::extcodehash:
  case AsmIdentifier::SpecialIdentifier::discard:
  case AsmIdentifier::SpecialIdentifier::discardu256:
  case AsmIdentifier::SpecialIdentifier::splitu256tou64:
  case AsmIdentifier::SpecialIdentifier::combineu64tou256:
  default:
    assert(false && "special function not supported yet");
    __builtin_unreachable();
  }
}

} // namespace soll::CodeGen
