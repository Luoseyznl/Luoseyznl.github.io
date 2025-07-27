// 用于扫描 posts 目录下所有 markdown 文件，提取 front matter 并生成 posts.json
// 用法：node generate-posts-json.js

const fs = require('fs');
const path = require('path');
const yaml = require('js-yaml');

const postsDir = path.join(__dirname, 'posts');
const output = path.join(__dirname, 'posts.json');

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
  return content.replace(/[#>*\-\[\]!`\n]/g, '').slice(0, 150) + (content.length > 150 ? '...' : '');
}

function main() {
  if (!fs.existsSync(postsDir)) {
    console.error('posts 目录不存在');
    process.exit(1);
  }
  const files = fs.readdirSync(postsDir).filter(f => f.endsWith('.md'));
  const posts = [];
  for (const file of files) {
    const md = fs.readFileSync(path.join(postsDir, file), 'utf-8');
    const { meta, content } = extractFrontMatter(md);
    if (!meta.title || !meta.date) {
      console.warn(`跳过 ${file}：缺少标题或日期`);
      continue; // 必须有标题和日期
    }
    posts.push({
      file: 'posts/' + file,
      title: meta.title,
      date: meta.date,
      category: meta.category || '',
      tags: meta.tags || [],
      summary: getSummary(content)
    });
  }
  // 按日期从新到旧排序
  posts.sort((a, b) => new Date(b.date) - new Date(a.date));
  fs.writeFileSync(output, JSON.stringify(posts, null, 2), 'utf-8');
  console.log('已生成 posts.json');
}

main();
