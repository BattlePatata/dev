#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

enum class TokenType {
    _return,
    int_lit,
    semi
};

struct Token {
    TokenType type;
    std::optional<std::string> value {};
};

std::vector<Token> tokenize(const std::string& str) {
    std::vector<Token> tokens;

    std::string buf;
    for (int idx = 0; idx < str.length(); idx++) {
        char chr = str.at(idx);
        if (std::isalpha(chr)) {
            buf.push_back(chr);
            idx++;
            while (std::isalnum(str.at(idx))) {
                buf.push_back(str.at(idx));
                idx++;
            }
            idx--;

            if (buf == "return") {
                tokens.push_back({.type = TokenType::_return});
                buf.clear();
                continue;
            } else {
                std::cerr << "You messed up!" << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (std::isdigit(chr)) {
            buf.push_back(chr);
            idx++;
            while (std::isdigit(str.at(idx))) {
                buf.push_back(str.at(idx));
                idx++;
            }
            idx--;
            tokens.push_back({.type = TokenType::int_lit, .value = buf});
            buf.clear();
        } else if (chr == ';') {
            tokens.push_back({.type = TokenType::semi});
        } else if (std::isspace(chr)) {
            continue;
        } else {
            std::cerr << "You messed up big time!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    return tokens;
}

std::string tokens_to_asm(const std::vector<Token>& tokens) {
    std::stringstream output;
    output << "global _start\n_start:\n";
    for (int idx = 0; idx < tokens.size(); idx++) {
        const Token& token = tokens.at(idx);
        if (token.type == TokenType::_return) {
            if (idx + 1 < tokens.size() && tokens.at(idx + 1).type == TokenType::int_lit) {
                if (idx + 2 < tokens.size() && tokens.at(idx + 2).type == TokenType::semi) {
                    output << "    mov rax, 60\n";
                    output << "    mov rdi, " << tokens.at(idx + 1).value.value() << "\n";
                    output << "    syscall";
                }
            }
        }
    }
    return output.str();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
        std::cerr << "hydro <input.hy>" << std::endl;
        return EXIT_FAILURE;
    }
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    std::vector<Token> tokens = tokenize(contents);
    {
        std::fstream file("out.asm", std::ios::out);
        file << tokens_to_asm(tokens);
    }

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
