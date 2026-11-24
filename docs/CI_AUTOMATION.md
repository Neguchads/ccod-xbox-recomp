English · [Português (Brasil)](#pt-br)

# CI / release automation — Stage 2

This repository contains **Windows recompilation project**.

## Browser tests
Not applicable: actual game execution requires a user-provided Xbox copy.

## Version/changelog proposals
Automatic versioned GitHub Releases are deliberately NOT enabled for this game port. Release packaging still requires a separate reviewed clean-room/native-build procedure.

## Source inventory and signed evidence
Use Actions → `Package integrity and SBOM (manual)` → `Run workflow`. This workflow is opt-in and never publishes a Release. The manual action records checksums and SPDX inventory of public source files only. It does NOT build or publish an Xbox game binary.

The resulting workflow artifact contains an SPDX-JSON SBOM, a SHA-256 checksum and a package/evidence archive. GitHub's attestation service signs the package's provenance and SBOM. Verify with GitHub CLI after downloading the archive:

```sh
gh attestation verify ./PACKAGE_NAME -R Neguchads/ccod-xbox-recomp
sha256sum -c PACKAGE_NAME.sha256
```

Replace PACKAGE_NAME with the archived filename and run the SHA-256 command in the directory containing the downloaded archive. Attestations prove *which workflow produced the bytes*, not that third-party licenses or program behavior are correct.

## Safety boundaries
- Never put credentials, real user content, proprietary ROMs, BIOS or generated game code into workflows.
- Each workflow uses a minimal applicable GitHub token permission set and SHA-pinned external actions.
- Human review remains mandatory before enabling a production release or uploading anything to a software store.
- For external websites or paid APIs, end-to-end smoke tests are intentionally limited to public or synthetic fixtures.

<a id="pt-br"></a>

# Automação da CI e distribuição — Etapa 2

A CI executa regressões sintéticas e valida a publicação sem usar arquivos de jogo.
O workflow manual **Package integrity and SBOM (manual)** calcula hashes SHA-256
e gera inventário SPDX e atestados assinados. Ele publica somente artefatos de
evidência no GitHub Actions, nunca um jogo, ISO, XBE ou binário recompilado.
Publicações oficiais continuam exigindo revisão humana e a mídia legítima
fornecida localmente pelo próprio usuário.
