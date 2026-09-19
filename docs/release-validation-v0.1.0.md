# LingNest v0.1.0 发布验证记录

验证日期：2026-09-18  
构建平台：Windows x64  
编译器：MSVC 19.44  
兼容运行时：Qt 5.15.2

## 已通过

- Debug 全量测试：15/15。
- Release 全量测试：15/15。
- 打包目录启动烟雾测试：退出码 0。
- EXE 文件版本、产品版本与应用版本：`0.1.0`。
- EXE、Qt 窗口和通知区域共用 `resources/icons/lingnest.ico`。
- `windeployqt` 运行库、QML 模块、平台插件和 SQLite 插件存在。
- VC++ x64 运行时采用应用本地 DLL 部署。
- 角色包只包含 `character.json`、`prompt.md` 与 `assets/` 运行资源。
- 发布扫描未发现 API Key、`config.json`、`memory.sqlite3`、日志、PDB、LIB、PEM 或 KEY 文件。
- ZIP SHA-256 与独立校验文件一致。
- 损坏配置恢复、损坏数据库恢复、数据库迁移、断网/超时、单实例、多屏边界算法和 API Key 脱敏均有自动化测试覆盖。

## 待人工跨环境签收

- 在一台未安装 Qt、Visual Studio 和 LingNest 的干净 Windows 10/11 x64 设备上启动。
- 在双显示器、不同缩放比例及热插拔场景下复核位置恢复与菜单钳制。
- 使用 Qt 6 正式工具链重建，验证系统 TLS/HTTPS 后再作为公网联网发行包。
- 正式公开发布前完成代码签名和角色/参考素材授权确认。

上述待办不影响本包作为首个本地发布候选与功能验收包使用，但它们是公开发行的签收条件。
