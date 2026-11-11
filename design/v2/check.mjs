import { access } from 'node:fs/promises';
import { readFile } from 'node:fs/promises';
for (const path of ['index.html','style.css','app.js','render.mjs','package.json']) await access(path);
const tokens = JSON.parse(await readFile('tokens.json', 'utf8'));
const required = ['background','surface','surfaceRaised','border','text','textMuted','accent','energy','harmonic','peak','warning','danger'];
for (const name of required) {
  if (!/^#[0-9a-f]{6}$/i.test(tokens.color[name])) throw new Error(`invalid color token: ${name}`);
}
for (const [name, value] of Object.entries(tokens.space)) if (!Number.isInteger(value) || value < 0) throw new Error(`invalid spacing token: ${name}`);
const qml = await readFile('../../src/desktop/qml/V2Tokens.qml', 'utf8');
for (const name of required) if (!qml.includes(`property string ${name}: \"${tokens.color[name]}\"`)) throw new Error(`QML drift: ${name}`);
const cli = await readFile('../../src/cli/v2_tokens.hpp', 'utf8');
for (const name of ['accent','energy','harmonic','peak','warning','danger']) if (!cli.includes(`${name} = \"${tokens.color[name]}\"`)) throw new Error(`CLI drift: ${name}`);
console.log('v2 design sources present');
