# CI & PR hygiene

- PR 模板：`.github/PULL_REQUEST_TEMPLATE.md`
- CI：`.github/workflows/ci.yml`（禁止跟踪构建产物；Ubuntu 上 `cmake --preset Debug` + build）
- 完整分支协作说明见 `docs/DEVELOPMENT.md`（`docs/dev-handbook` 分支）
