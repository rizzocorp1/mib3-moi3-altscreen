# stubs/rsi-mockup — dados nossos para o mock RSI da LG

O firmware traz um mock de serviços RSI (`/lge/app_ro/bin/rsi-mockup`) com dados de teste em
`/lge/app_ro/sai/mockup/`. Esta pasta é copiada **por cima** desses dados quando o lançador roda
com `RSI_DATA=/stubs/rsi-mockup` (ver `../../driver/m1_hmi.sh`). Cada arquivo é
`<serviço>/<recurso>.json`, uma lista de elementos. É estudo num computador; nada vai para o carro.

| Arquivo | O que é | Por que existe |
|---|---|---|
| `usermanagement/userContexts.json` | o contexto de usuário "vehicle" | O da LG só traz `activeUser`, e como referência (`id`, `name`, `uri`). A tela Welcome da HMI exige também `identifiedUser`, igual ao ativo, e os dois **com** `authenticationLevel`. O mock não expande referências (`$expand` volta igual), então o usuário vai embutido por inteiro. |
| `onlineregistration/primaryUsers.json` | um usuário principal com `nickname` vazio | A HMI só libera a tela Welcome depois de receber um elemento de `primaryUsers` com o campo `nickname` presente. Vazio quer dizer "este carro não tem usuário principal". O mock da LG não traz dados deste serviço. |

O usuário usado é o "level2 user" (`secondaryUser`) que já está em
`/lge/app_ro/sai/mockup/usermanagement/users.json`, o mesmo que a LG pôs como `activeUser`. Os
identificadores do contexto e do usuário são os da LG; o do usuário principal foi gerado aqui.

Os serviços entram na lista do mock pela variável `RSI_SERVICES` do lançador
(`usermanagement:auth onlineregistration:communication`). A categoria depois dos dois pontos é
só um rótulo: a HMI a usa na tela de privacidade, não para achar o serviço (conferido no código
decompilado, `RsiHandler` e `RootSubscriptions`). A categoria real desses dois serviços no carro
não foi verificada.

**Desde as 3h35 de 12/10/2026 o mock serve também o serviço de áudio** (`audiomanagement:mixer`),
quando o lançador roda com `SOM`, que o modo ao vivo liga. Os dados desse serviço não ficam nesta
pasta: estão em [../som/](../som/README.md) e entram na pasta do mock já com o perfil do "som de
mentira" aplicado. Com `SOM` o lançador monta ainda, por cima da especificação desse serviço e só
dentro do container, uma cópia afrouxada em dois pontos. O que foi medido do comportamento do mock
(busca por nome, gravação, o que ele recusa, quando avisa quem assina) está no Marco 13 do
`modelagem-soc.md`, em "O mock da LG: o que ele faz e o que não deixa".

Registro da investigação: [../../modelagem-soc.md](../../modelagem-soc.md), seção do Marco 4.
