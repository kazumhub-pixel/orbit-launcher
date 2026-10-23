# Aurora Launcher

Fork independente do Prism Launcher 11.1.1, sem vínculo ou endosso do projeto Prism. Código sob GPL-3.0-only; licenças e créditos originais preservados em COPYING.md e LICENSE.

## Escopo desta primeira versão

- Núcleo C++/Qt do Prism mantido: instâncias, versões, Java, mods e importação de modpacks.
- Identidade Aurora e pasta de dados separada da instalação padrão do Prism.
- Três temas nativos: Aurora Dark, Light e Forest.
- Editor em Configurações > Aparência > Personalizar tema Aurora: cinco cores, fonte, tamanho, cantos e espaçamento, com prévia.
- Importação e exportação de temas Aurora em JSON. O tema instalado também usa o formato nativo JSON/QSS do Prism, preservando a compatibilidade com temas da comunidade.
- Perfis locais separados das contas Microsoft. O modo local não afirma propriedade do jogo nem cria autenticação Microsoft.

## Contas e servidores

Perfis locais usam o fluxo offline existente e podem entrar nos servidores que aceitam jogadores nesse modo. O servidor decide sua política de autenticação. Um perfil local não fornece sessão oficial, skins oficiais, proteção de apelido ou acesso aos servidores que exigem uma conta Microsoft com Minecraft comprado.

O código de login Microsoft foi preservado. Para distribuir a autenticação em nome do Aurora é necessário registrar um aplicativo próprio e compilar com Launcher_MSA_CLIENT_ID. A chave do CurseForge também está vazia; seu recurso de catálogo depende de uma chave própria. Modrinth e importação de arquivos são caminhos independentes dessas credenciais. Nenhum login Microsoft foi automatizado.

A identidade local não é uma conta em nuvem. Um futuro serviço próprio com senha, recuperação, sessões, skins e administração de servidores é um componente separado.

## Temas

Para começar, escolha um dos três temas em Aparência. O editor cria uma cópia nova ao salvar; não sobrescreve temas da comunidade. Compartilhe o arquivo *.aurora.json pelo botão Exportar. Importe o arquivo no editor e use Salvar e aplicar para instalá-lo.

Os arquivos de tema ficam na pasta themes do diretório de dados do Aurora. Cada tema criado tem um identificador aleatório e contém theme.json, themeStyle.css e aurora.json. Nomes de tema não são usados como caminhos de pasta.

## Compilar no Windows

Dependências equivalentes às do upstream: MSYS2 CLANG64, clang/lld, CMake, Ninja, extra-cmake-modules, Qt6 base/svg/imageformats/networkauth, cmark, qrencode, tomlplusplus e libarchive. Use JDK17 para os auxiliares Java; isso não impede selecionar outras versões de Java nas instâncias.

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DLauncher_QT_VERSION_MAJOR=6 \
  -DLauncher_APP_BINARY_NAME=AuroraLauncher \
  -DLauncher_UPDATER_GITHUB_REPO= \
  -DLauncher_BUILD_ARTIFACT= \
  -DLauncher_ENABLE_JAVA_DOWNLOADER=ON \
  -DBUILD_TESTING=ON
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
```

Não habilite o atualizador do Prism neste fork. Configure um canal próprio quando houver releases Aurora publicadas.

## O que ainda precisa de validação

Consulte VALIDACAO.md para os resultados reais do build, testes e execução desta entrega. Não há benchmark que comprove menor consumo de RAM que o Prism original. O uso de Qt nativo evita adicionar uma camada de navegador, mas a comparação de desempenho exige medição.


Correção de 22/10/2026: iniciar normalmente com um perfil local permite baixar metadados, bibliotecas e assets sem autenticação Microsoft. A opção explícita de iniciar offline continua sem atualizar esses arquivos. A identidade enviada ao jogo permanece local. Bibliotecas nativas da instância de teste 26.1.2 recuperadas dos URLs oficiais com tamanho e SHA-1 conferidos; execução do jogo ainda aguarda teste humano.
