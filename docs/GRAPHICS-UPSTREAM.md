# Graphics integration from jmafort

English · [Português (Brasil)](#pt-br)

Reference: `jmafort/CastlevaniaCOD-PC` commit
`00408f145232c2593fef47499979adac5024ba98` (v1.14.1).
This is an integration backlog, not a list of delivered features.

Stage 1 is implemented: compiled shaders are stored under `cache/shaders` beside
the executable. `XBE_SHADER_CACHE` can select another folder. Cache failures fall
back to compilation; the cache is disposable and contains compiled host shaders,
not saves. On 2026-11-27 the owner reported normal gameplay after the cache change; no performance gain was measured.


Stage 4a adds Xbox-hash PNG textures through WIC and generates mipmaps for BMP/PNG once on load. DDS data and Portuguese pack priority are preserved. PCSX2 filename matching, normal/ORM/height maps and material shading remain pending.

## Scope and order

| Stage | Work | Acceptance |
|---|---|---|
| 1 | Persistent compiled-shader cache | A second process reads the original bytecode; damaged files regenerate; a read-only directory leaves compilation working. |
| 2 | Reconcile SDK graphics interfaces | Preserve local gameplay hooks, diagnostics, translation and frame skipping. Keep the shared SDK untouched. |
| 3 | Shadows, transparency, viewport and display | Compare the same scenes; correct character shadows, masks and HUD across aspect ratios. |
| 4 | Texture import and material maps | Preserve BMP/BC7 packs and Portuguese priority; add PCSX2 names, normal/ORM/height maps and parallax. |
| 5 | Temporal rendering and upscalers | Correct motion vectors, depth, jitter, history resets and UI separation before TAA, FSR or DLSS. |
| 6 | HDR and frame generation | Verify swapchain colour formats, presentation, resize and speedhack interactions on supported hardware. |
| 7 | Ambient occlusion and DXR | Integrate geometry capture, acceleration structures and screen-space fallbacks; test capability detection. |
| 8 | HD movie replacement | Preserve the original timing and Portuguese voices; load external movie packs. |

## Provenance and boundaries

The donor README explicitly licenses `sdk-patches` under the XboxXBERecomp MIT
license. Its other port source is GPLv2. Check provenance per file: a feature's
name does not establish the license of all its implementation. Vendor DLLs and
texture/movie packs retain their own terms and are not downloaded by this work.

The current SDK is based on `e0de3e44`; the donor targets `91146ff`. The local
renderer includes a private copy of the older runtime, so merely applying the
donor patch to an SDK does not upgrade the renderer that this project builds.
The shader cache is the first independent integration point. The other stages
require interface reconciliation rather than a wholesale patch application.

Only the owner opens and tests the game. Unit tests and a successful build do
not certify visual equivalence, full gameplay or a performance improvement.

---

<a id="pt-br"></a>

# Integração gráfica do jmafort

Referência: `jmafort/CastlevaniaCOD-PC`, commit
`00408f145232c2593fef47499979adac5024ba98` (v1.14.1).
Esta é a fila de integração, não uma lista de recursos entregues.

Etapa 1 implementada: shaders compilados ficam em `cache/shaders`, ao lado do
executável. `XBE_SHADER_CACHE` permite escolher outra pasta. Falha no cache volta
à compilação; ele é descartável e contém shaders do host, não saves. Desempenho
ainda não foi medido. Em 27/11/2026 o dono relatou que o jogo rodou normalmente após a mudança do cache.


Etapa 4a adiciona PNG pelo hash Xbox via WIC e gera mipmaps de BMP/PNG uma vez no carregamento. DDS e prioridade do pacote PT-BR ficam preservados. Pareamento de nomes PCSX2, mapas normal/ORM/altura e iluminação de materiais continuam pendentes.

## Escopo e ordem

| Etapa | Trabalho | Critério de aceitação |
|---|---|---|
| 1 | Cache persistente de shaders compilados | Outro processo lê o bytecode original; arquivo danificado regenera; pasta sem escrita não impede compilação. |
| 2 | Conciliar interfaces gráficas do SDK | Preservar hooks, diagnóstico, tradução e frameskip. Não alterar o SDK compartilhado. |
| 3 | Sombra, transparência, viewport e tela | Comparar as mesmas cenas; conferir sombra, máscaras e HUD em diferentes proporções. |
| 4 | Importação de texturas e materiais | Preservar BMP/BC7 e prioridade do português; adicionar nomes PCSX2, mapas normal/ORM/altura e parallax. |
| 5 | Renderização temporal e upscalers | Acertar movimento, profundidade, jitter, reinício do histórico e separação da interface antes de TAA, FSR ou DLSS. |
| 6 | HDR e geração de quadros | Conferir formatos de cor, apresentação, redimensionamento e speedhack em hardware compatível. |
| 7 | Oclusão ambiente e DXR | Integrar captura de geometria, estruturas de aceleração e alternativas sem ray tracing; testar detecção de suporte. |
| 8 | Substituição de vídeos HD | Preservar tempo original e vozes PT-BR; carregar pacotes externos. |

## Origem e limites

O README do donor licencia `sdk-patches` em MIT, do XboxXBERecomp; o restante
do código do port está em GPLv2. Conferir por arquivo: o nome de um recurso
não determina a licença de toda a implementação. DLLs de fornecedores e packs
de texturas/vídeos têm termos próprios e não são baixados por este trabalho.

O SDK atual parte de `e0de3e44`; o donor usa `91146ff`. O renderizador local
inclui uma cópia do runtime antigo: aplicar o patch só no SDK não atualiza
o renderizador compilado pelo projeto. O cache é o primeiro ponto independente;
as outras etapas exigem conciliar interfaces, não aplicar todo o patch às cegas.

Só o dono abre e testa o jogo. Testes isolados e build aprovado não comprovam
equivalência visual, campanha completa nem melhoria de desempenho.
