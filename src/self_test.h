#pragma once

// Runs the headless self-test suite. When exportBmp is true, also writes
// verification screenshots to the current directory. Returns 0 on success.
int runSelfTestAndExportScreenshots(bool exportBmp);
