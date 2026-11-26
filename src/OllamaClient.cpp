#include "OllamaClient.h"

#include <chrono>
#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

void Configure(httplib::Client& c, int readSec) {
    c.set_connection_timeout(3, 0);
    c.set_read_timeout(readSec, 0);
    c.set_write_timeout(10, 0);
}

std::string DescribeError(httplib::Error e) {
    switch (e) {
    case httplib::Error::Connection:
        return "Ollama に接続できません。Ollama が起動しているか確認してください。";
    case httplib::Error::Read:
        return "応答がタイムアウトしました。モデルの読み込みに時間がかかっている可能性があります。";
    default:
        return std::string("通信エラー: ") + httplib::to_string(e);
    }
}

// 失敗レスポンスの {"error": "..."} を取り出す
std::string ExtractApiError(const std::string& body) {
    try {
        auto j = json::parse(body);
        if (j.contains("error") && j["error"].is_string()) return j["error"].get<std::string>();
    } catch (...) {}
    return {};
}

// 推論系モデルが出す <think>...</think> を除去
std::string StripThink(std::string s) {
    const std::string open = "<think>", close = "</think>";
    auto b = s.find(open);
    if (b != std::string::npos) {
        auto e = s.find(close, b);
        if (e != std::string::npos) s.erase(b, e + close.size() - b);
    }
    auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

}  // namespace

ModelsResult OllamaClient::ListModels() const {
    ModelsResult r;
    try {
        httplib::Client cli(endpoint_);
        Configure(cli, 10);
        auto res = cli.Get("/api/tags");
        if (!res) { r.error = DescribeError(res.error()); return r; }
        if (res->status != 200) {
            r.error = "モデル一覧の取得に失敗しました (HTTP " + std::to_string(res->status) + ")";
            return r;
        }
        auto j = json::parse(res->body);
        for (auto& m : j.at("models")) r.models.push_back(m.at("name").get<std::string>());
        r.ok = true;
    } catch (const std::exception& e) {
        r.error = std::string("応答の解析に失敗しました: ") + e.what();
    }
    return r;
}

TranslateResult OllamaClient::Translate(const std::string& model,
                                        const std::string& systemPrompt,
                                        const std::string& userText) const {
    TranslateResult r;
    const auto t0 = std::chrono::steady_clock::now();
    try {
        json body = {
            {"model", model},
            {"stream", false},
            {"messages", json::array({
                {{"role", "system"}, {"content", systemPrompt}},
                {{"role", "user"},   {"content", userText}}})},
            {"options", {{"temperature", 0.2}}}};

        httplib::Client cli(endpoint_);
        Configure(cli, 300);
        auto res = cli.Post("/api/chat",
                            body.dump(-1, ' ', false, json::error_handler_t::replace),
                            "application/json");
        if (!res) {
            r.error = DescribeError(res.error());
        } else if (res->status == 404) {
            r.error = "モデル '" + model + "' が見つかりません。`ollama pull " + model + "` で導入してください。";
        } else if (res->status != 200) {
            auto msg = ExtractApiError(res->body);
            r.error = "翻訳に失敗しました (HTTP " + std::to_string(res->status) + ")" +
                      (msg.empty() ? "" : ": " + msg);
        } else {
            auto j = json::parse(res->body);
            r.text = StripThink(j.at("message").at("content").get<std::string>());
            r.ok = true;
        }
    } catch (const std::exception& e) {
        r.error = std::string("応答の解析に失敗しました: ") + e.what();
    }
    r.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return r;
}
