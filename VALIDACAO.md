# Validação — Aurora Launcher / primeira base

## Implementado
- Fork do Prism11.1.1, commit upstream7d2d3c1ec6fcf8255ebacd02d9f792688575dfb4.
- Nome, identificador e íconeAurora; diretório de dados independente; atualizador doPrism desativado.
- Temas nativosDark/Light/Forest, editorvisualQt com prévia, cinco cores, fonte, tamanho, cantos e espaçamento.
- Importar/exportar pacoteAuroraJSON e gerar tema nativoPrism JSON/QSS com gravação por arquivos temporários e pastaUUID.
- Criar e selecionar perfis locais sem adicionar uma contaMicrosoft primeiro; fluxo de lançamentoOffline existente reaproveitado sem afirmar propriedadeMinecraft.
- ContaMicrosoft preservada no código, mas desativada nesta compilação por faltar um clientID próprio doAurora. CurseForge sem chave própria; integração dependente dessa chave não configurada.

## Testado
- CompilaçãoRelease Windowsx64 com Clang23.1.3 e Qt6.12.0: passou, com avisosC++ tratados comoerro.
- JDK17.0.20.1 utilizado para os auxiliaresJava do launcher; Java25 do usuário não foi alterado.
- CTest:22/22executáveis de teste passaram; resultado final em2,22s aproximadamente (consulte o relatórioCTestraw para o tempo preciso).
- Novos testes: round-trip de pacote de tema, rejeição de cor/espaçamento inválidos, instalação sem transformar nomes em caminhos, identidade local serializada sem propriedadeMinecraft e controles/preset do editor.
- EditorQt renderizado emprocesso de teste separado sem erros de estilo. Captura ../Aurora-theme-preview.png.
- Executávelportátil da primeira montagem abriu a configuração inicial e a janela principal, observadas por automaçãoWindows. Ao detectar interação do usuário, a automação de entrada foi interrompida.
- Pacoteportátil final limpo: AuroraLauncher.exe --version retornou código0 e AuroraLauncher11.1.1 sem depender doPATH doambienteMSYS2.

## Não testado / pendente
- Download e execução de Minecraft, instalação de mods/modpacks e conexão a servidores nesta compilação.
- LoginMicrosoft e catálogoCurseForge, dependentes de identificadores próprios.
- Comparação deRAM/CPU/tempo deinício com Prismoriginal. Um processo da primeira montagem foi observado em~175MiB deworking set num instante; isso não é um benchmark comparativo e não estabelece consumo típico.
- Serviço próprio de contas emnuvem, senhas, recuperação, skins e infraestrutura deautenticação. Perfislocais desta versão são identidadesoffline.
- Assinaturadigital, instalador.exe e atualizaçõesAurora publicadas.

## Executar
Extraia todo o ZIPportátil para uma pasta gravável e executeAuroraLauncher.exe. Não copie apenas oEXE: mantenha asDLLs e subpastas junto dele. EmConfigurações>Aparência ficam os temas e oeditor; emContas ficaAdicionar perfil local. Perfislocais só entram em servidoresque aceitam esse modo; os que exigem autenticaçãooficial continuam exigindo contaMicrosoftválida.

O código mantém as licenças e créditosPrism/PolyMC/MultiMC. Não há vínculo ou endosso dessesprojetos.


Correção de 22/10/2026: iniciar normalmente com um perfil local permite baixar metadados, bibliotecas e assets sem autenticação Microsoft. A opção explícita de iniciar offline continua sem atualizar esses arquivos. A identidade enviada ao jogo permanece local. Bibliotecas nativas da instância de teste 26.1.2 recuperadas dos URLs oficiais com tamanho e SHA-1 conferidos; execução do jogo ainda aguarda teste humano.
