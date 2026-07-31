const { chmodSync, existsSync, readdirSync } = require('fs');
const { join } = require('path');

if (process.platform === 'win32') {
  process.exit(0);
}

const roots = [
  join(__dirname, '..', 'node_modules', 'node-pty', 'prebuilds'),
  join(__dirname, '..', '..', 'node-pty', 'prebuilds'),
];

for (const root of roots) {
  if (!existsSync(root)) continue;
  for (const dir of readdirSync(root)) {
    const helper = join(root, dir, 'spawn-helper');
    try {
      chmodSync(helper, 0o755);
    } catch {
      // ignore missing or unreadable helpers
    }
  }
}