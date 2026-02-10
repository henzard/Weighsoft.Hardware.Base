/**
 * Backend sends timestamp as millis() (ms since ESP32 boot), not Unix epoch.
 * Format for display as "Boot + X.X s" or "Boot + N ms".
 */
export function formatSerialTimestamp(msSinceBoot: number): string {
  if (msSinceBoot >= 1000) {
    const sec = (msSinceBoot / 1000).toFixed(1);
    return `Boot + ${sec} s`;
  }
  return `Boot + ${msSinceBoot} ms`;
}
