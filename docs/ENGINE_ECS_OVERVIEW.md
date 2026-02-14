# DX11 Study Engine - Visão Geral da Arquitetura ECS

## 1) Objetivo Atual

O projeto evoluiu de um renderer orientado a assets para um mini engine com ECS em DirectX 11.

Objetivos já implementados:

- IDs de entidade seguros com geração/versão.
- Ciclo de vida de entidades (criação e destruição).
- Armazenamento de componentes cache-friendly com array denso + lookup esparso.
- Transform como componente ECS.
- Camadas de atualização de scene e renderização de scene.
- Renderer consumindo lista de entidades e matrizes world.
- Model/View/Projection no shader via constant buffer.

---

## 2) Estrutura do Projeto (Partes ECS)

- `include/ecs/Entity.h`
- `include/ecs/EntityManager.h`
- `include/ecs/ComponentSystem.h`
- `src/ecs/EntityManager.cpp`

- `include/scene/Scene.h`
- `include/scene/SceneRenderer.h`
- `include/scene/components/TransformComponent.h`
- `include/scene/components/MeshRendererComponent.h`
- `include/scene/systems/TransformSystem.h`
- `src/scene/Scene.cpp`
- `src/scene/SceneRenderer.cpp`
- `src/scene/systems/TransformSystem.cpp`

- `include/renderer/Dx11Renderer.h`
- `src/renderer/Dx11Renderer.cpp`

- `include/core/CameraController.h`
- `src/core/CameraController.cpp`

- `src/main.cpp`

---

## 3) Sistema de Entidades

### 3.1 Formato do Entity ID

`ecs::Entity` é um inteiro de 32 bits dividido em:

- bits baixos: índice da entidade
- bits altos: geração/versão

Arquivo:

- `include/ecs/Entity.h`

Esse formato evita handles antigos inválidos:

- se a entidade `E` é destruída e o índice é reutilizado, a geração muda
- o handle antigo não passa mais em `IsAlive`

### 3.2 EntityManager

Arquivos:

- `include/ecs/EntityManager.h`
- `src/ecs/EntityManager.cpp`

Operações implementadas:

- `Create()`
- `Destroy(entity)`
- `IsAlive(entity)`
- `Clear()`

Internos:

- `generations_`
- `aliveFlags_`
- `freeIndices_`

---

## 4) Sistema de Componentes (Cache Friendly)

Arquivo:

- `include/ecs/ComponentSystem.h`

### 4.1 Estratégia de Storage

Cada tipo de componente usa um `ComponentStorage<T>`:

- `components_` (array denso)
- `entities_` (array denso paralelo)
- `sparse_` (entity-index -> dense index + 1)

Benefícios:

- iteração rápida em dados densos (bom para cache)
- `add/get/has/remove` em O(1) médio
- remoção com swap-and-pop sem buracos

### 4.2 API Pública

No `ComponentSystem`:

- `Add<T>(entity, component)`
- `Get<T>(entity)`
- `Has<T>(entity)`
- `Remove<T>(entity)`
- `OnEntityDestroyed(entity)`
- `TryGetStorage<T>()`

---

## 5) Camada de Scene

### 5.1 Componentes

Arquivos:

- `include/scene/components/TransformComponent.h`
- `include/scene/components/MeshRendererComponent.h`

Componentes:

- `TransformComponent`
  - `local: math::Transform`
  - `world: math::Mat4`
- `MeshRendererComponent`
  - `assetPath`
  - `visible`

### 5.2 Sistemas

Arquivo:

- `src/scene/systems/TransformSystem.cpp`

`TransformSystem::Update` calcula:

- `world = local.ToMatrix()`

### 5.3 Container de Scene

Arquivos:

- `include/scene/Scene.h`
- `src/scene/Scene.cpp`

Responsabilidades:

- ciclo de vida de entidades
- posse de componentes
- atualização da scene
- construção da lista de render

Métodos principais:

- `CreateEntity()`
- `DestroyEntity(entity)`
- `Update()`
- `BuildRenderList()`

`BuildRenderList()` gera itens `RenderEntity` com:

- entity id
- asset path
- world matrix

### 5.4 Scene Renderer

Arquivo:

- `src/scene/SceneRenderer.cpp`

Responsabilidade:

- chamar `scene.Update()`
- passar lista de render para o renderer DX11

---

## 6) Integração do Renderer com ECS

Arquivos:

- `include/renderer/Dx11Renderer.h`
- `src/renderer/Dx11Renderer.cpp`

Agora o renderer:

- inicializa com contexto DX11 + serviços de asset/material
- recebe `std::vector<scene::RenderEntity>` por frame
- cacheia recursos de malha na GPU por `assetPath`
- calcula por entidade `MVP = world * view * projection`
- envia MVP no constant buffer do vertex shader (`b0`)

Ponto central da integração ECS:

- renderer não é dono dos transforms da scene
- transform vem da scene ECS

---

## 7) Separação da Lógica de Câmera

Arquivos:

- `include/core/CameraController.h`
- `src/core/CameraController.cpp`

Comportamento da câmera fora do renderer:

- mouse look (botão direito)
- rotação por teclado (`W/A/S/D`)
- atualização de `math::Camera`

O renderer só usa as matrizes finais da câmera para o MVP.

---

## 8) Persistência de Scene

Arquivos:

- `include/assets/SceneRepository.h`
- `src/assets/SceneRepository.cpp`

Persistência atual em JSON:

- nome do asset
- dados de transform (position/rotation/scale)

No `main`, o JSON é carregado para entidades/componentes ECS e salvo de volta a partir do ECS.

---

## 9) Fluxo de Editor Implementado

Fluxo de inicialização:

1. Browser de projetos (`projects/`)
2. Hub de scenes (selecionar/criar/apagar scene + escolher asset para placement)
3. Runtime

No runtime:

- clique esquerdo instancia o asset selecionado como nova entidade
- scene é persistida em JSON

Arquivos:

- `src/editor/ProjectBrowserWindow.cpp`
- `src/editor/SceneHubWindow.cpp`
- `src/main.cpp`

---

## 10) Build

Target validado:

- `dx11_m0`

Comando:

```powershell
cmake --build build_repair_vs --config Debug --target dx11_m0
```

---

## 11) Tradeoffs (Didático vs Produção)

Escolhas didáticas:

- update ECS single-thread
- sem hierarquia parent/child de transform ainda
- sem etapa separada de extração para RenderWorld

Escolhas mais próximas de engine real:

- entity id seguro com geração
- storage denso com sparse set
- renderer consumindo world matrix externo
- cache de malhas GPU por asset path

---

## 12) Próximos Passos Recomendados

1. Adicionar assinaturas/arquetipos para filtros de sistemas com múltiplos componentes.
2. Introduzir `CameraComponent` e câmera como entidade.
3. Implementar hierarquia parent/child e propagação de world transform.
4. Adicionar `MaterialComponent` e binding de textura/material por entidade.
5. Adicionar frustum culling antes da submissão ao render.
6. Criar gizmo no editor para mover/rotacionar/escalar entidades selecionadas.
