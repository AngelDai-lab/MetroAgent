#ifndef CLI_MAIN_H
#define CLI_MAIN_H

// CLI 模式入口：接收命令行参数，输出 JSON 到 stdout，日志走 stderr
// 供 Python Agent 通过 subprocess 调用
int runCli(int argc, char** argv);

#endif // CLI_MAIN_H
