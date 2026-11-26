#pragma once
#include <string>
#include <vector>

struct ModelsResult {
    bool ok = false;
    std::vector<std::string> models;
    std::string error;  // UTF-8
};

struct TranslateResult {
    bool ok = false;
    std::string text;   // UTF-8
    std::string error;  // UTF-8
    double seconds = 0.0;
};

// 同期API。呼び出し側（ワーカースレッド）でブロックして良い。
class OllamaClient {
public:
    explicit OllamaClient(std::string endpoint) : endpoint_(std::move(endpoint)) {}

    ModelsResult ListModels() const;
    TranslateResult Translate(const std::string& model,
                              const std::string& systemPrompt,
                              const std::string& userText) const;

private:
    std::string endpoint_;
};
