#pragma once
#include <string>
#include <vector>

struct LangItem   { const char* label; const char* name; };        // UTF-8 表示名 / プロンプト用英語名
struct PresetItem { const char* label; const char* instruction; }; // UTF-8 表示名 / 指示文

namespace PromptBuilder {
const std::vector<LangItem>& SourceLanguages();  // 先頭は「自動判定」
const std::vector<LangItem>& TargetLanguages();
const std::vector<PresetItem>& Presets();

std::string BuildSystemPrompt(int srcIdx, int dstIdx, int presetIdx);
}
