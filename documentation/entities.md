# Modelo de Entidades de Dados

Este documento descreve a estrutura e o propósito das principais entidades de dados utilizadas no Simulador de Livro de Ofertas.


## Entidades de Domínio (Domain Entities)
### Entidade: Ordem (Order)

A entidade `Ordem` representa uma intenção ou um pedido enviado por um cliente para comprar ou vender um ativo. É um objeto dinâmico e com estado, cujo ciclo de vida é gerenciado pelo `Matching Engine`.

#### Atributos

| Atributo | Tipo de Dado (Conceitual) | Descrição |
| :--- | :--- | :--- |
| `order_id` | Identificador Único | Número exclusivo que identifica a ordem dentro do sistema. |
| `client_id` | Identificador | Identifica o cliente/participante que enviou a ordem. |
| `client_order_id` | Identificador | Identificador único da ordem do ponto de vista do cliente. |
| `symbol` | Texto | O código do ativo a ser negociado (ex: "PETR4"). |
| `side` | Enumerado | O lado da ordem: `COMPRA` (Buy) ou `VENDA` (Sell). |
| `type` | Enumerado | O tipo da ordem: `LIMIT` (com preço) ou `MARKET` (a mercado). |
| `status` | Enumerado | O estado atual da ordem no seu ciclo de vida. |
| `time_in_force` | Enumerado | A regra de validade da ordem (ex: `DAY`, `GTC`, `IOC`). |
| `capacity` | Enumerado | A capacidade do participante (ex: `AGENCY`, `PRINCIPAL`). |
| `quantity` | Objeto Numérico | Contém a **quantidade original** e a **quantidade restante** a ser executada. |
| `price` | Número (Ponto Flutuante) | O preço limite da ordem (aplicável apenas para ordens `LIMIT`). |
| `timestamp` | Data/Hora | O momento exato em que a ordem foi aceita pelo `Matching Engine`. |

#### Ciclo de Vida / Ações

* **Ser Criada:** A ordem é instanciada a partir de um `NewOrderCommand`. Seu estado inicial é `NEW` e sua quantidade restante é igual à original.
* **Ser Executada:** Após um cruzamento no `Matching Engine`, a ordem é atualizada. Sua `quantidade restante` diminui, e seu `status` pode mudar para `PARTIALLY_FILLED` ou `FILLED`.
* **Ser Cancelada:** A partir de um `CancelOrderCommand`, o `status` da ordem é alterado para `CANCELED`, e ela é removida do `Order Book`.
* **Ser Modificada:** A partir de um `AmendOrderCommand`, seus atributos como `quantity` ou `price` podem ser alterados.

---

### Entidade: Negócio (Trade)

A entidade `Negócio` representa um fato histórico e imutável. É o registro de uma transação que ocorreu em um ponto específico no tempo, como resultado do cruzamento de duas ordens.

#### Atributos

| Atributo | Tipo de Dado (Conceitual) | Descrição |
| :--- | :--- | :--- |
| `trade_id` | Identificador Único | Número exclusivo que identifica esta transação específica. |
| `symbol` | Texto | O código do ativo que foi negociado. |
| `price` | Número (Ponto Flutuante) | O preço exato em que o negócio foi fechado. |
| `quantity` | Número | A quantidade de ações que foram trocadas nesta transação. |
| `timestamp` | Data/Hora | O momento exato em que o cruzamento ocorreu. |
| `aggressor_order_id` | Identificador | O ID da ordem que iniciou a transação (a que chegou por último). |
| `passive_order_id` | Identificador | O ID da ordem que já estava no livro e foi executada. |

#### Ciclo de Vida / Ações

* **Ser Criado:** Um `Trade` é criado uma única vez pelo `Matching Engine` no momento exato em que um cruzamento de ordens acontece.
* **Ser Consultado:** Após sua criação, um `Trade` é imutável. Suas informações podem ser apenas lidas/consultadas, mas nunca alteradas.

---

## Entidades de Mensageria (Messaging Entities)

Estas entidades são os veículos de comunicação transientes que transportam informações entre os componentes do sistema.

### Comandos (Commands)

Comandos representam uma intenção ou um pedido para alterar o estado do sistema. Eles fluem do `Inbound Gateway` para o `Matching Engine`.

| Comando | Propósito | Atributos Principais |
| :--- | :--- | :--- |
| `NewOrderCommand` | Solicitar a criação de uma nova ordem no livro. | Contém todos os dados de uma nova `Ordem`. |
| `CancelOrderCommand`| Solicitar o cancelamento de uma ordem existente. | `order_id`: O ID da ordem a ser cancelada. |
| `AmendOrderCommand` | Solicitar a modificação de uma ordem ativa. | `order_id`: O ID da ordem a ser modificada, e os novos dados (ex: nova quantidade, novo preço). |

### Eventos (Events)

Eventos são objetos imutáveis que representam um fato que já ocorreu no sistema.

#### Eventos Transacionais (para a `Execution Queue`)

| Evento | Propósito | `ExecType` Gerado |
| :--- | :--- | :--- |
| `OrderAccepted` | Notificar que uma ordem foi aceita e está no livro com status `NEW`. | `0` (New) |
| `OrderRejected` | Notificar que uma ordem foi rejeitada na entrada. | `8` (Rejected) |
| `TradeExecuted` | Notificar que um negócio foi executado. Causa a mudança de status da(s) ordem(ns) para `PARTIALLY_FILLED` ou `FILLED`. | `F` (Trade) |
| `OrderPendingCancel`| Notificar que um pedido de cancelamento foi aceito e está sendo processado. |`6` (Pending Cancel)|
| `OrderCanceled` | Notificar que um pedido de cancelamento foi bem-sucedido. | `4` (Canceled) |
| `OrderAmended` | Notificar que uma ordem foi modificada com sucesso. | `5` (Replaced) |

#### Eventos de Dados de Mercado (para o `Market Data Channel`)

| Evento | Propósito | Atributos Principais |
| :--- | :--- | :--- |
| `BookSnapshotEvent` | Publicar uma "foto" completa do estado do livro de ofertas em um determinado momento. | `symbol`, uma lista de Bids (`preço`, `quantidade_agregada`), uma lista de Asks (`preço`, `quantidade_agregada`). |
