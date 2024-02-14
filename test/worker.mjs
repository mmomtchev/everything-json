import * as path from 'node:path';
import * as fs from 'node:fs';
import { fileURLToPath } from 'node:url';
import { JSON as JSONAsync } from 'everything-json';

export default async function () {
  const dir = path.resolve(fileURLToPath(import.meta.url), '..', 'data');
  const files = await fs.promises.readdir(dir);
  const texts = await Promise.all(files.filter((file) => file.match(/json$/))
    .map((file) => fs.promises.readFile(path.resolve(dir, file), 'utf-8')));

  const q = [];
  for (let i = 0; i < texts.length * 2; i++) {
    const idx = i % texts.length;
    q.push(JSONAsync.parseAsync(texts[idx])
      .then((json) => json.toObjectAsync()));
  }
  return Promise.all(q);
}
