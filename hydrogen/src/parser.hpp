#pragma once

#include <variant>
#include <cassert>

#include "./arena.hpp"
#include "tokenization.hpp"

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;
struct NodeTermParen {
    NodeExpr* expr;
};

// Using arena allocation
struct NodeBinExprAdd {
    NodeExpr* lft_hnd_side;
    NodeExpr* rght_hnd_side;
};

struct NodeBinExprSub {
    NodeExpr* lft_hnd_side;
    NodeExpr* rght_hnd_side;
};

struct NodeBinExprDiv {
    NodeExpr* lft_hnd_side;
    NodeExpr* rght_hnd_side;
};

struct NodeBinExprMulti {
    NodeExpr* lft_hnd_side;
    NodeExpr* rght_hnd_side;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprSub*, NodeBinExprMulti*, NodeBinExprDiv*> var;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};
// End of arena allocation

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr{};
};

struct NodeStmt;
struct NodeScope {
    std::vector<NodeStmt*> stmts;
};

struct NodeIfPred;
struct NodeIfPredElif {
    NodeExpr* expr{};
    NodeScope* scope{};
    std::optional<NodeIfPred*> pred;
};

struct NodeIfPredElse {
    NodeScope* scope;
};

struct NodeIfPred {
    std::variant<NodeIfPredElif*, NodeIfPredElse*> var;
};

struct NodeStmtIf {
    NodeExpr* expr{};
    NodeScope* scope{};
    std::optional<NodeIfPred*> pred;
};

struct NodeStmtAssign {
    Token ident;
    NodeExpr* expr {};
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeScope*, NodeStmtIf*, NodeStmtAssign*> var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) 
        : m_tokens(std::move(tokens))
        , m_allocator(1024 * 1024 * 4) { // 4mb
    }

    std::optional<NodeBinExpr*> parse_bin_expr() {
        if (auto lft_hnd_side = parse_expr()) {
            auto bin_expr = m_allocator.emplace<NodeBinExpr>();
            if (peek().has_value() && peek().value().type == TokenType::plus) {
                auto bin_expr_add = m_allocator.emplace<NodeBinExprAdd>();
                bin_expr_add->lft_hnd_side = lft_hnd_side.value();
                consume();
                if (auto rght_hnd_side = parse_expr()) {
                    bin_expr_add->rght_hnd_side = rght_hnd_side.value();
                    bin_expr->var = bin_expr_add;
                    return bin_expr;
                }
                else {
                    std::cerr << "Expected expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else {
                std::cerr << "Unsupported binary operator" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else {
            return {};
        }
    }

    std::optional<NodeTerm*> parse_term() {
        if (auto int_lit = try_consume(TokenType::int_lit)) {
            auto term_int_lit = m_allocator.emplace<NodeTermIntLit>(int_lit.value());
            auto term = m_allocator.emplace<NodeTerm>(term_int_lit);

            return term;
        }
        if (auto ident = try_consume(TokenType::ident)) {
            auto term_ident = m_allocator.emplace<NodeTermIdent>(ident.value());
            auto term = m_allocator.emplace<NodeTerm>(term_ident);
            return term;
        }
        if (auto open_paren = try_consume(TokenType::open_paren)) {
            auto expr = parse_expr();
            if (!expr.has_value()) {
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)`");
            auto term_paren = m_allocator.emplace<NodeTermParen>(expr.value());
            auto term = m_allocator.emplace<NodeTerm>(term_paren);
            return term;
        }
        return {};
    }

    std::optional<NodeExpr*> parse_expr(const int min_prec = 0) {
        std::optional<NodeTerm*> term_lft_hnd_side = parse_term();
        if (!term_lft_hnd_side.has_value()) {
            return {};
        }
        auto expr_lft_hnd_side = m_allocator.emplace<NodeExpr>(term_lft_hnd_side.value());
        while(true) {
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;
            if (curr_tok.has_value()) {
                prec = bin_prec(curr_tok->type);
                if (!prec.has_value() || prec < min_prec) {
                    break;
                }
            }
            else {
                break;
            }
            const auto [type, value] = consume();
            const int next_min_prec = prec.value() + 1;
            auto expr_rght_hnd_side = parse_expr(next_min_prec);
            if (!expr_rght_hnd_side.has_value()) {
                std::cerr << "Unable to parse expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto expr = m_allocator.emplace<NodeBinExpr>();
            auto expr_lft_hnd_side_v2 = m_allocator.emplace<NodeExpr>();
            if (type == TokenType::plus) {
                expr_lft_hnd_side_v2->var = expr_lft_hnd_side->var;
                auto add = m_allocator.emplace<NodeBinExprAdd>(expr_lft_hnd_side_v2, expr_rght_hnd_side.value());
                expr->var = add;
            }
            else if (type == TokenType::star) {
                expr_lft_hnd_side_v2->var = expr_lft_hnd_side->var;
                auto multi = m_allocator.emplace<NodeBinExprMulti>(expr_lft_hnd_side_v2, expr_rght_hnd_side.value());
                expr->var = multi;
            } 
            else if (type == TokenType::minus) {
                expr_lft_hnd_side_v2->var = expr_lft_hnd_side->var;
                auto sub = m_allocator.emplace<NodeBinExprSub>(expr_lft_hnd_side_v2, expr_rght_hnd_side.value());
                expr->var = sub;
            } 
            else if (type == TokenType::fslash) {
                expr_lft_hnd_side_v2->var = expr_lft_hnd_side->var;
                auto div = m_allocator.emplace<NodeBinExprDiv>(expr_lft_hnd_side_v2, expr_rght_hnd_side.value());
                expr->var = div;
            } 
            else {
                assert(false); // Unreachable;
            }
            expr_lft_hnd_side->var = expr;
        }
        return expr_lft_hnd_side;
    }

    std::optional<NodeScope*> parse_scope() {
        if (!try_consume(TokenType::open_curly).has_value()) {
            return {};
        }
        auto scope = m_allocator.emplace<NodeScope>();
        while (auto stmt = parse_stmt()) {
            scope->stmts.push_back(stmt.value());
        }
        try_consume(TokenType::close_curly, "Expected `}`");
        return scope;
    }

    std::optional<NodeIfPred*> parse_if_pred() {
        if (try_consume(TokenType::elif)) {
            try_consume(TokenType::open_paren, "Expected `(`");
            const auto elif_ = m_allocator.alloc<NodeIfPredElif>();
            if (const auto expr = parse_expr()) {
                elif_->expr = expr.value();
            }
            else {
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)`");
            if (const auto scope = parse_scope()) {
                elif_->scope = scope.value();
            }
            else {
                std::cerr << "Expected scope" << std::endl;
                exit(EXIT_FAILURE);
            }
            elif_->pred = parse_if_pred();
            auto pred = m_allocator.emplace<NodeIfPred>(elif_);
            return pred;
        }
        if (try_consume(TokenType::else_)) {
            auto else_ = m_allocator.alloc<NodeIfPredElse>();
            if (const auto scope = parse_scope()) {
                else_->scope = scope.value();
            }
            else {
                std::cerr << "Expected scope" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto pred = m_allocator.emplace<NodeIfPred>(else_);
            return pred;
        }
        return {};
    }

    std::optional<NodeStmt*> parse_stmt() {
        if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
            consume();
            consume();
            auto stmt_exit = m_allocator.emplace<NodeStmtExit>();
            if (const auto node_expr = parse_expr()) {
                stmt_exit->expr = node_expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)`");
            try_consume(TokenType::semi, "Expected `;`");
            auto stmt = m_allocator.emplace<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        } 
        if (peek().has_value() && peek().value().type == TokenType::let
            && peek(1).has_value() && peek(1).value().type == TokenType::ident
            && peek(2).has_value() && peek(2).value().type == TokenType::eq) {

            consume();
            auto stmt_let = m_allocator.emplace<NodeStmtLet>();
            stmt_let->ident = consume();
            consume();
            if (const auto expr = parse_expr()) {
                stmt_let->expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::semi, "Expected `;`");
            auto stmt = m_allocator.emplace<NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
        }
        if (peek().has_value() && peek().value().type == TokenType::ident 
            && peek(1).has_value() && peek(1).value().type == TokenType::eq) {
            const auto assign = m_allocator.alloc<NodeStmtAssign>();
            assign->ident = consume();
            consume();
            if (const auto expr = parse_expr()) {
                assign->expr = expr.value();
            }
            else {
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::semi, "Expected `;`");
            const auto stmt = m_allocator.emplace<NodeStmt>(assign);
            return stmt;
        }
        if (peek().has_value() && peek().value().type == TokenType::open_curly) {
            if (auto scope = parse_scope()) {
                auto stmt = m_allocator.emplace<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            }
            std::cerr << "Invalid scope" << std::endl;
            exit(EXIT_FAILURE);
        }
        if (const auto if_ = try_consume(TokenType::if_)) {
            try_consume(TokenType::open_paren, "Expected `(`");
            auto stmt_if = m_allocator.emplace<NodeStmtIf>();
            if (auto expr = parse_expr()) {
                stmt_if->expr = expr.value();
            }
            else {
                std::cerr << "Invalid if expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)`");
            if (const auto scope = parse_scope()) {
                stmt_if->scope = scope.value();
            }
            else {
                std::cerr << "Invalid scope" << std::endl;
                exit(EXIT_FAILURE);
            }
            stmt_if->pred = parse_if_pred();
            auto stmt = m_allocator.emplace<NodeStmt>(stmt_if);
            stmt->var = stmt_if;
            return stmt;
        }
        return {};
    }

    std::optional<NodeProg> parse_prog() {
        NodeProg prog;
        while (peek().has_value()) {
            if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            }
            else {
                std::cerr << "Invalid statement" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return prog;
    }

private:
    [[nodiscard]] std::optional<Token> peek(const size_t offset = 0) const {
        if (m_curr_idx + offset >= m_tokens.size()) {
            return {};
        }
        return m_tokens.at(m_curr_idx + offset);
    }

    Token consume() {
        return m_tokens.at(m_curr_idx++);
    }

    Token try_consume(const TokenType type, const std::string& err_msg) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        std::cerr << err_msg << std::endl;
        exit(EXIT_FAILURE);
    }

    std::optional<Token> try_consume(const TokenType type) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        return {};
    }

    const std::vector<Token> m_tokens;
    size_t m_curr_idx = 0;
    ArenaAllocator m_allocator;
};
