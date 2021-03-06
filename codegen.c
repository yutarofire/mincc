#include "mincc.h"

Function *current_fn;

int count = 0;

void gen_addr(Node *node) {
  if (node->kind != ND_VAR)
    error("expected a variable node");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->var->offset);
  printf("  push rax\n");
}

void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->kind == ND_VAR) {
    gen_addr(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_ASSIGN) {
    gen_addr(node->lhs);
    gen_expr(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  if (node->kind == ND_FUNCALL) {
    printf("  call %s\n", node->funcname);
    printf("  push rax\n");
    return;
  }

  // lhs on RAX
  // rhs on RDI
  gen_expr(node->lhs);
  gen_expr(node->rhs);
  printf("  pop rdi\n");
  printf("  pop rax\n");

  if (node->kind == ND_ADD) {
    printf("  add rax, rdi\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_SUB) {
    printf("  sub rax, rdi\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_MUL) {
    printf("  imul rax, rdi\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_DIV) {
    printf("  cqo\n");
    printf("  idiv rdi\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_EQ) {
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_NEQ) {
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_LT) {
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    printf("  push rax\n");
    return;
  }

  error("expression expected");
}

void gen_stmt(Node *node) {
  if (node->kind == ND_EXPR_STMT) {
    gen_expr(node->lhs);
    printf("  pop rax\n");
    return;
  }

  if (node->kind == ND_RETURN) {
    gen_expr(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .L.return.%s\n", current_fn->name);
    return;
  }

  if (node->kind == ND_IF) {
    count = count + 1;
    int c = count;

    gen_expr(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .L.else.%d\n", c);

    gen_stmt(node->then);
    printf("  jmp .L.end.%d\n", c);

    printf(".L.else.%d:\n", c);
    if (node->els)
      gen_stmt(node->els);

    printf(".L.end.%d:\n", c);
    return;
  }

  if (node->kind == ND_FOR) {
    count = count + 1;
    int c = count;

    if (node->init) {
      gen_expr(node->init);
      printf("  pop rax\n");
    }

    printf(".L.then.%d:\n", c);
    gen_stmt(node->then);

    if (node->inc) {
      gen_expr(node->inc);
      printf("  pop rax\n");
    }

    if (node->cond) {
      gen_expr(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.end.%d\n", c);
    }

    printf("  jmp .L.then.%d\n", c);
    printf(".L.end.%d:\n", c);
    return;
  }

  if (node->kind == ND_BLOCK) {
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;
  }

  error("statment expected");
}

int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

void codegen(Function *prog) {
  printf("  .intel_syntax noprefix\n");

  for (Function *fn = prog; fn; fn = fn->next) {
    current_fn = fn;

    int stack_size = 0;
    for (Var *v = fn->lvars; v; v = v->next)
      if (stack_size < v->offset)
        stack_size = v->offset;
    stack_size = align_to(stack_size, 16);

    printf("  .globl %s\n", fn->name);
    printf("%s:\n", fn->name);

    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", stack_size);

    for (Node *n = fn->body; n; n = n->next)
      gen_stmt(n);

    // epilogue
    printf(".L.return.%s:\n", fn->name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}
