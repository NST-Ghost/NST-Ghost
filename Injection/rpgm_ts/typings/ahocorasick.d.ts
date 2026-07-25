// typings/ahocorasick.d.ts
// Aho-Corasick 多模式字符串匹配算法库类型定义

declare module "ahocorasick" {
  /**
   * 搜索结果类型
   * 每个元素是一个元组：[endIndex, matchedPatterns]
   * - endIndex: 匹配结束位置的索引（包含）
   * - matchedPatterns: 在该位置匹配到的所有模式数组
   */
  export type SearchResult = Array<[number, string[]]>;

  /**
   * Aho-Corasick 自动机类
   * 用于高效的多模式字符串匹配
   */
  export default class AhoCorasick {
    /**
     * 创建 Aho-Corasick 自动机实例
     * @param patterns 要匹配的模式字符串数组
     */
    constructor(patterns: string[]);

    /**
     * 在文本中搜索所有匹配的模式
     * @param text 要搜索的文本
     * @returns 匹配结果数组，每个元素为 [endIndex, matchedPatterns[]]
     */
    search(text: string): SearchResult;
  }
}