# Diorama Espacial 3D Interativo

Projeto de avaliação final da disciplina de **Computação Gráfica** — curso de Ciência da Computação (modalidade híbrida) da Unisinos. O código-fonte principal é `src/GrauB.cpp`.

Trata-se de um diorama interativo ambientado em uma superfície lunar, composto por modelos 3D de um terreno com crateras, um disco voador, um alienígena, uma bandeira hasteada e painéis solares. O usuário explora a cena livremente em primeira pessoa, seleciona e edita qualquer objeto ao vivo, configura a iluminação de três pontos e anima modelos ao longo de trajetórias definidas por Curvas de Bézier.

---

## Funcionalidades Principais

- **Câmera FPS** — navegação livre com WASD + mouse, pitch limitado a ±89°, zoom via scroll da rodinha.
- **Iluminação Phong de 3 Pontos** — *Key Light* (principal, branca intensa), *Fill Light* (preenchimento, difusa suave) e *Back Light* (contraluz), todas independentemente ativáveis em tempo real.
- **Sistema de Animação com Curvas de Bézier** — algoritmo de De Casteljau de grau N. O usuário grava pontos de controle a partir da posição da câmera e o objeto selecionado percorre a curva em loop com interpolação suave via função seno.
- **Leitor Customizado de OBJ/MTL** — carrega vértices, normais, coordenadas UV, materiais por grupos (`usemtl`) e texturas (`map_Kd`), com triangulação automática de polígonos arbitrários (*fan triangulation*) e suporte a modelos multi-material.
- **Editor de Cena Ao Vivo** — seleção de objetos por ciclo, translação, rotação e escala em tempo real com modo de precisão (SHIFT). O estado completo da cena é persistido em `cena.txt` com um único pressionamento de tecla.
- **Destaque de Seleção** — o objeto ativo recebe um tint vermelho no fragment shader para identificação visual imediata.

---

## Tecnologias Utilizadas

| Tecnologia | Função |
|---|---|
| C++17 | Linguagem principal |
| OpenGL 4.1 Core Profile | Pipeline de renderização |
| GLFW 3 | Janela, contexto e eventos de entrada |
| GLAD | Carregador de extensões OpenGL |
| GLM | Matemática de vetores e matrizes |
| stb_image | Decodificação de texturas PNG/JPG |

---

## Controles — Manual de Instruções

### Câmera

| Tecla / Dispositivo | Ação |
|---|---|
| `W` `A` `S` `D` | Mover a câmera (frente / esquerda / trás / direita) |
| Mouse | Girar a câmera (look around) |
| Scroll do mouse | Zoom (ajusta o campo de visão) |

### Seleção e Edição de Objetos

> Segure `LEFT SHIFT` em qualquer operação de edição para ativar o **modo de precisão** (velocidade reduzida a 1/10).

| Tecla | Ação |
|---|---|
| `TAB` | Selecionar o próximo objeto (ciclo circular) |
| `I` / `K` | Mover o objeto no eixo Z (frente / trás) |
| `J` / `L` | Mover o objeto no eixo X (esquerda / direita) |
| `Y` / `H` | Mover o objeto no eixo Y (cima / baixo) |
| `Seta Cima` / `Seta Baixo` | Rotacionar no eixo X |
| `Seta Esq` / `Seta Dir` | Rotacionar no eixo Y |
| `Z` / `C` | Rotacionar no eixo Z |
| `[` | Diminuir a escala do objeto |
| `]` | Aumentar a escala do objeto |
| `ENTER` | Salvar o estado atual da cena em `cena.txt` |

### Iluminação e Visualização

| Tecla | Ação |
|---|---|
| `1` | Ligar / desligar a Key Light (luz principal) |
| `2` | Ligar / desligar a Fill Light (luz de preenchimento) |
| `3` | Ligar / desligar a Back Light (contraluz) |
| `T` | Alternar entre textura de imagem e cor do material `.mtl` |
| `O` | Ligar / desligar o destaque vermelho de seleção |

### Animação por Curvas de Bézier

O sistema permite definir uma trajetória livre para qualquer objeto em três passos:

| Tecla | Ação |
|---|---|
| `P` | Grava a posição atual da câmera como ponto de controle da curva |
| `SPACE` | Liga / desliga a animação do objeto selecionado (mínimo de 3 pontos) |
| `BACKSPACE` | Limpa todos os pontos de controle e para a animação |

**Fluxo típico:** selecione um objeto com `TAB` → caminhe até o primeiro ponto e pressione `P` → repita para mais 2 ou mais posições → pressione `SPACE` para iniciar o movimento em loop.

### Sistema

| Tecla | Ação |
|---|---|
| `ESC` | Fechar a aplicação |

---

## Estrutura e Execução

### Arquivo de Cena (`cena.txt`)

A cena é descrita em um arquivo de texto simples, um objeto por linha, no formato:

```
OBJETO <caminho/para/modelo.obj> <pos_x> <pos_y> <pos_z> <escala> [rot_x rot_y rot_z]
```

Exemplo:

```
OBJETO assets/chao.obj         0.0  -1.0  0.0  1.0
OBJETO assets/Low_poly_UFO.obj 0.0   2.0  0.0  0.3  0.0 45.0 0.0
OBJETO assets/alien.obj        0.5  -0.8  1.2  0.15
```

Ao pressionar `ENTER` durante a execução, a aplicação sobrescreve `cena.txt` com as posições, escalas e rotações atuais de todos os objetos, persistindo qualquer edição feita ao vivo.

### Dependências de Assets

Todos os assets (`.obj`, `.mtl`, `.png`, `.jpg`) devem estar acessíveis a partir do **diretório de trabalho do executável** (pasta `build/`). Os caminhos registrados em `cena.txt` são relativos a esse diretório. A estrutura esperada dentro de `build/` é:

```
build/
├── GrauB.exe        (ou GrauB no Linux/macOS)
├── cena.txt
└── assets/
    ├── chao.obj
    ├── chao.mtl
    ├── chao.jpg
    ├── Low_poly_UFO.obj
    └── ...
```

### Compilação (CMake)

Antes de compilar, é necessário baixar a GLAD manualmente no [GLAD Generator](https://glad.dav1d.de/) com as configurações **API: OpenGL / Version: 4.1 / Profile: Core / Language: C/C++** e copiar os arquivos gerados para:

- `glad.h` → `include/glad/`
- `khrplatform.h` → `include/glad/KHR/`
- `glad.c` → `common/`

Após configurar as dependências:

```bash
# Na raiz do repositório
mkdir build && cd build
cmake ..
cmake --build . --config Release
./GrauB
```

Consulte [GettingStarted.md](GettingStarted.md) para um tutorial detalhado de configuração.

---

---

## Procedência dos Assets e Processamento Prévio

Os modelos 3D que compõem o diorama foram obtidos gratuitamente em plataformas de distribuição sob licenças *Royalty Free* e de Domínio Público para uso educacional. 

**Processamento Prévio:** Alguns dos modelos não puderam ser utilizado no estado "cru" ("raw"). Foi necessário realizar o processamento prévio utilizando softwares e editores de texto para compatibilidade com o motor gráfico:
* **Autodesk Maya / Cinema 4D (Resquícios):** Alguns modelos vieram com exportações sujas (ex: Arnold Renderer `aiStandardSurface`) e caracteres ocultos de quebra de linha (`\r`).
* **Edição Manual de MTL:** Os arquivos `.mtl` foram limpos e reescritos manualmente para o modelo de iluminação de Phong clássico (ajustando `Kd`, `Ka`, `Ks`), além de mapear corretamente os caminhos para as texturas `.png` e `.jpg`.

| Modelo | Plataforma | Autor original |
|---|---|---|
| Terreno Lunar (com crateras) | [CGTrader](https://www.cgtrader.com/3d-models/space/planet/moon-mountain-4-with-8k-textures) | Desertsage Seals |
| Disco Voador (`Low_poly_UFO`) | [CGTrader](https://www.cgtrader.com/free-3d-models/space/spaceship/free-flying-saucer) | jonlundy3d |
| Alienígena | [Free3D](https://free3d.com/3d-model/grayalien-v01--560376.html) | printable_models |
| Bandeira Hasteada | [CGTrader](https://www.cgtrader.com/items/4830611/download-page) | aliali25 |
| Placa Solar | [CGTrader](https://www.cgtrader.com/items/2475823/download-page) | renderbunny |

---

## Bibliografia, Documentação e Tutoriais Consultados

Para o desenvolvimento do pipeline gráfico, leitor de objetos e cálculos matemáticos aplicados neste semestre, as seguintes referências foram fundamentais:

* **LearnOpenGL:** Principal material de apoio para a compreensão do pipeline programável moderno, cálculo de transformações de matrizes e implementação do modelo de iluminação de Phong. ([learnopengl.com](https://learnopengl.com/))
* **Documentação Oficial Khronos Group:** Consulta às especificações oficiais do OpenGL 4.1 e da linguagem GLSL (OpenGL Shading Language). ([opengl.org](https://www.opengl.org/))
* **Documentação GLM (OpenGL Mathematics):** Utilizada para a compreensão das funções matemáticas vetoriais e geração de matrizes de projeção (`glm::perspective`) e visualização (`glm::lookAt`). ([glm.g-truc.net](https://glm.g-truc.net/))
* **stb_image.h (Sean Barrett):** Documentação oficial da biblioteca utilizada para o carregamento e decodificação das texturas 2D (PNG/JPG). ([github.com/nothings/stb](https://github.com/nothings/stb))
* **Algoritmo de De Casteljau:** Pesquisas e referências matemáticas em fóruns de computação gráfica para a implementação recursiva e iterativa das Curvas Paramétricas de Bézier.

