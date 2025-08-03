// 用于扫描 posts 目录下所有 markdown 文件，提取 front matter 并生成 posts.json
// 用法：node generate-posts-json.js

const fs = require("fs");
const path = require("path");
const yaml = require("js-yaml");

const postsDir = path.join(__dirname, "posts");
const output = path.join(__dirname, "posts.json");

function extractFrontMatter(md) {
  const match = md.match(/^---([\s\S]+?)---/);
  if (match) {
    const meta = yaml.load(match[1]);
    const content = md.slice(match[0].length).trim();
    return { meta, content };
  }
  return { meta: {}, content: md };
}

function getSummary(content) {
  // 寻找非标题、非目录的第一段有意义文本
  const paragraphs = content.split(/\n\s*\n/).filter(p => p.trim().length > 0);
  
  // 跳过标题、分隔符和目录
  let firstParagraph = '';
  for (const p of paragraphs) {
    const trimmed = p.trim();
    if (!trimmed.startsWith('#') && 
        !trimmed.startsWith('---') && 
        !trimmed.startsWith('- [') &&
        !trimmed.match(/^目录|^Contents|^Table of contents/i)) {
      firstParagraph = trimmed;
      break;
    }
  }
  
  // 如果没找到合适段落，取第一段非空内容
  if (!firstParagraph && paragraphs.length > 0) {
    firstParagraph = paragraphs[0];
  }
  
  // 移除 Markdown 标记和数学公式
  let summary = firstParagraph
    .replace(/[#>*\-\[\]!`\n]/g, '')
    .replace(/\$\$(.*?)\$\$/g, '')
    .replace(/\$(.*?)\$/g, '')
    .trim();
    
  return summary.length > 150 ? summary.slice(0, 150) + '...' : summary;
}

function main() {
  if (!fs.existsSync(postsDir)) {
    console.error("posts 目录不存在");
    process.exit(1);
  }
  const files = fs.readdirSync(postsDir).filter((f) => f.endsWith(".md"));
  const posts = [];
  for (const file of files) {
    const md = fs.readFileSync(path.join(postsDir, file), "utf-8");
    const { meta, content } = extractFrontMatter(md);
    if (!meta.title || !meta.date) {
      console.warn(`跳过 ${file}：缺少标题或日期`);
      continue; // 必须有标题和日期
    }
    posts.push({
      file: "posts/" + file,
      title: meta.title,
      date: meta.date,
      category: meta.category || "",
      tags: meta.tags || [],
      summary: getSummary(content),
      cover: meta.cover || "", // 新增：封面图
      author: meta.author || "葛蔚", // 新增：作者
      readingTime: Math.ceil(content.length / 500) + "分钟", // 简单估算阅读时长
    });
  }
  // 按日期从新到旧排序
  posts.sort((a, b) => new Date(b.date) - new Date(a.date));
  fs.writeFileSync(output, JSON.stringify(posts, null, 2), "utf-8");
  console.log("已生成 posts.json");
}

main();
