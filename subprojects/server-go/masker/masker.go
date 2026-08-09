package masker

import (
	"fmt"
	"regexp"
	"strings"
)

// Combined RPG Maker / Game Escape Code Regular Expression
var rpgmControlRegex = regexp.MustCompile(`(?i)(` +
	`\\\\[VNP]\[\d+\]` +
	`|\\\\I\[\d+\]` +
	`|\\\\C\[\d+\]` +
	`|\\\\G` +
	`|\\\\[{}]` +
	`|\\\\\$` +
	`|\\\\[.|]` +
	`|\\\\!` +
	`|\\\\[><]` +
	`|\\\\\\^` +
	`|\\\\\\\\` +
	`|\\\\FS\[\d+\]` +
	`|\\\\P[XY]\[-?\d+\]` +
	`|\\\\[OT]C\[\d+\]` +
	`|\\\\(?:MSGCORE|MSGSND)\[[^\]]*\]` +
	`|\\[VNP]\[\d+\]` +
	`|\\[IC]\[\d+\]` +
	`|\\[G!^\$]` +
	`|\\[{}]` +
	`|\\[.|]` +
	`|\\FS\[\d+\]` +
	`|\\P[XY]\[-?\d+\]` +
	`|\\[OT]C\[\d+\]` +
	`|\\(?:MSGCORE|MSGSND)\[[^\]]*\]` +
	`)`)

type MaskResult struct {
	MaskedText string
	TagMap     map[string]string // Tag "__NST_TAG_0__" -> Original Code "\v[1]"
}

// Mask replaces control codes in source text with safe placeholders
func Mask(sourceText string) MaskResult {
	if sourceText == "" {
		return MaskResult{MaskedText: sourceText, TagMap: make(map[string]string)}
	}

	tagMap := make(map[string]string)
	matches := rpgmControlRegex.FindAllString(sourceText, -1)

	tagIndex := 0
	for _, code := range matches {
		alreadyMapped := false
		for _, mappedCode := range tagMap {
			if mappedCode == code {
				alreadyMapped = true
				break
			}
		}

		if !alreadyMapped {
			tag := fmt.Sprintf("__NST_TAG_%d__", tagIndex)
			tagIndex++
			tagMap[tag] = code
		}
	}

	maskedText := sourceText
	for tag, code := range tagMap {
		maskedText = strings.ReplaceAll(maskedText, code, tag)
	}

	return MaskResult{
		MaskedText: maskedText,
		TagMap:     tagMap,
	}
}

// Unmask restores original control codes from tagMap in translated text
func Unmask(translatedText string, tagMap map[string]string) string {
	if len(tagMap) == 0 || translatedText == "" {
		return translatedText
	}

	text := translatedText

	for tag, originalCode := range tagMap {
		// 1. Direct replacement
		text = strings.ReplaceAll(text, tag, originalCode)

		// 2. Flexible replacement for spaces or case mutations introduced by LLM/Google
		tagNum := strings.TrimPrefix(tag, "__NST_TAG_")
		tagNum = strings.TrimSuffix(tagNum, "__")

		flexPattern := fmt.Sprintf(`(?i)__\s*NST\s*_\s*TAG\s*_\s*%s\s*__`, regexp.QuoteMeta(tagNum))
		flexRegex, err := regexp.Compile(flexPattern)
		if err == nil {
			text = flexRegex.ReplaceAllString(text, originalCode)
		}
	}

	return text
}
