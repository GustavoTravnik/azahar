# Guia de Plugins Dinâmicos (C ABI) para o Azahar Emulator

O **Azahar** suporta uma arquitetura modular de plugins nativos de alto desempenho baseada em **C ABI (`extern "C"`)**. Isso permite que desenvolvedores criem extensões em **C**, **C++**, **Rust** ou **Zig** que interajam diretamente com o ciclo de vida da emulação, savestates e logging sem sobrecarga de bindings interpretados.

---

## 1. Estrutura da API (`plugin_api.h`)

Para criar um plugin, inclua o cabeçalho `common/plugin_api.h` (ou copie suas definições de estruturas para o seu projeto).

### Metadados e Lifecycle Hooks (Exportados pelo seu Plugin)

| Função | Assinatura C | Descrição |
|---|---|---|
| `on_plugin_get_info` | `const PluginInfo* on_plugin_get_info(void)` | Retorna nome, versão, autor e descrição exibidos na interface Qt. |
| `on_plugin_init` | `int on_plugin_init(const EmulatorAPI* api)` | Invocado quando o plugin é carregado. Recebe os ponteiros de função do Azahar. |
| `on_frame_rendered` | `void on_frame_rendered(void)` | Invocado a cada frame de vídeo concluído pelo emulador. |
| `on_interval_tick` | `void on_interval_tick(void)` | Invocado periodicamente (a cada ~50ms) de forma assíncrona. |
| `on_plugin_shutdown` | `void on_plugin_shutdown(void)` | Invocado antes de o plugin ser descarregado da memória. |

*(Nota: Funções prefixadas com `azahar_plugin_*` também são aceitas como alternativa).*

### Funções Fornecidas pelo Emulador (`EmulatorAPI`)

Através do ponteiro `const EmulatorAPI* api` recebido em `on_plugin_init`:
- `api->save_state_slot(int slot_index)`: Salva o estado de emulação no slot especificado (1 a 10). Retorna `0` em sucesso.
- `api->load_state_slot(int slot_index)`: Carrega o estado de emulação do slot especificado (1 a 10). Retorna `0` em sucesso.
- `api->log_message(int level, const char* message)`: Emite uma mensagem para o sistema de logs do Azahar (`PLUGIN_LOG_INFO`, `PLUGIN_LOG_WARN`, `PLUGIN_LOG_ERROR`, etc.).
- `api->get_emulation_state()`: Retorna o estado atual: `EMU_STATE_STOPPED` (0), `EMU_STATE_RUNNING` (1) ou `EMU_STATE_PAUSED` (2).
- `api->get_current_slot()`: Retorna o slot de savestate ativo no emulador.

---

## 2. Como Compilar um Plugin

### Opção A: Usando MSVC (`cl.exe` no Windows)
Abra o *x64 Native Tools Command Prompt for VS 2022* e execute:
```cmd
cl /LD /O2 /I "caminho\para\azahar\src" meu_plugin.c /Fe:meu_plugin.dll
```

### Opção B: Usando GCC / MinGW
```bash
gcc -O2 -shared -fPIC -I "caminho/para/azahar/src" meu_plugin.c -o meu_plugin.dll
```

### Opção C: Usando Clang / Linux (`.so`)
```bash
clang -O2 -shared -fPIC -I "caminho/para/azahar/src" meu_plugin.c -o meu_plugin.so
```

### Opção D: Usando CMake
Exemplo de `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.20)
project(meu_plugin LANGUAGES C)

add_library(meu_plugin SHARED meu_plugin.c)
target_include_directories(meu_plugin PRIVATE "caminho/para/azahar/src")
```

---

## 3. Instalando e Ativando no Azahar

1. **Abrir o Diretório de Plugins**:
   - Abra o Azahar.
   - Acesse o menu **Tools > Plugins...** (ou **File > Open Azahar Folder > Plugins Folder**).
   - Clique no botão **"Open Plugins Folder"**. A pasta de plugins do emulador será aberta no Explorer.
2. **Copiar o Binário**:
   - Cole o arquivo gerado (`meu_plugin.dll` no Windows ou `meu_plugin.so` no Linux) dentro da pasta `plugins/`.
3. **Ativar o Plugin**:
   - Na janela **Plugins**, clique em **"Reload Plugins"**.
   - O seu plugin aparecerá listado com seu Nome, Versão e Descrição.
   - Marque a caixa de seleção para **Habilitar (Enabled)**.
   - O estado habilitado/desabilitado é salvo automaticamente nas configurações.

---

## 4. Exemplo Funcional: Auto-Save Circular

Confira a pasta [examples/auto_save_plugin/](file:///r:/Github/Desktop/Games/azahar-plugins-adapter/examples/auto_save_plugin/) para um exemplo completo de plugin que salva automaticamente o jogo a cada 5 minutos nos slots 1 a 10 em rotação circular.
