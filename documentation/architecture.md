## Descrição dos Componentes da Arquitetura
Esta documentação descreve as responsabilidades e interações de cada componente principal do Simulador de Livro de Ofertas.

### 1. Inbound Gateway

* **Responsabilidade Principal:** Servir como a porta de entrada do sistema. É responsável por receber, registrar para segurança e traduzir as requisições dos clientes em comandos internos.
* **Entradas:** Comandos brutos em formato de texto (simulando o protocolo FIX).
* **Processamento:**
    1.  **Registro (Write-Ahead Log):** Imediatamente escreve uma cópia exata do comando bruto recebido no `Input Log`.
    2.  **Interpretação:** Analisa o comando para entender a intenção do cliente (ex: `NEW_ORDER`, `CANCEL_ORDER`).
    3.  **Validação:** Se a mensagem for inválida (ex: falta um campo obrigatório, formato incorreto), ele gera um `OrderRejectedEvent`.
    4.  **Criação de Comando (Fábrica):** Se a mensagem for válida, instancia o objeto de "Comando" apropriado (ex: `NewOrderCommand`), preenchendo-o com os dados da requisição.
* **Saídas:** Um "objeto de comando" para a `Thread Safe Inbound Queue` ou um "objeto de evento" (`OrderRejectedEvent`) para a `Execution Queue` (via `Event Bus`).

### 2. Input Log

* **Responsabilidade Principal:** Prover um registro durável e cronológico de todas as requisições que entraram no sistema, para fins de "replay" (re-execução) e recuperação de desastres.
* **Entradas:** Strings de texto com os comandos brutos, enviadas pelo `Inbound Gateway`.
* **Processamento:** Simplesmente anexa a string recebida a um arquivo de texto em disco.
* **Saídas:** O arquivo `InputLog.txt`.

### 3. Thread Safe Inbound Queue

* **Responsabilidade Principal:** Desacoplar o `Inbound Gateway` do `Matching Engine`, agindo como uma "bandeja de entrada" segura e ordenada para as tarefas (comandos) a serem executadas.
* **Entradas:** Objetos de "Comando" (`NewOrderCommand`, `CancelOrderCommand`, etc.).
* **Processamento:** Armazena os comandos em uma estrutura FIFO (First-In, First-Out), protegida por um `mutex`.
* **Saídas:** Objetos de "Comando", um por um, para o `Matching Engine`.

### 4. Matching Engine

* **Responsabilidade Principal:** O cérebro do sistema. Orquestra a execução de todos os comandos, aplica a lógica de negócio e mantém o estado do mercado através do `Order Book`.
* **Entradas:** Objetos de "Comando" da `Thread Safe Inbound Queue`.
* **Processamento:**
    1.  Retira um `Comando` da fila e o executa.
    2.  Toda a interação com o `Order Book` (consultas, inserções, remoções) é realizada aqui.
    3.  A cada mudança de estado ou ação significativa, gera um ou mais objetos de "Evento" (`OrderAccepted`, `TradeExecuted`, `BookUpdateEvent`, etc.) para descrever o resultado.
* **Saídas:** Objetos de "Evento" que são **publicados** no `Event Bus / Dispatcher`.

### 5. Order Book

* **Responsabilidade Principal:** Manter o estado atual e ordenado de todas as ordens ativas de compra (Bids) e venda (Asks). É o "banco de dados em memória" do `Matching Engine`.
* **Entradas:** Requisições de inserção, remoção e consulta vindas do `Matching Engine`.
* **Processamento:** Organiza as ordens seguindo a regra de prioridade Preço-Tempo.
* **Saídas:** Respostas às consultas do `Matching Engine`.

### 6. Event Bus / Dispatcher

* **Responsabilidade Principal:** Agir como um roteador de eventos central. Desacopla completamente o `Matching Engine` dos vários canais de saída.
* **Entradas:** Todos os objetos de "Evento" publicados pelo `Matching Engine`.
* **Processamento:** Analisa o **tipo** de cada evento recebido e o direciona para a fila correta.
* **Saídas:**
    * Eventos transacionais (`OrderAccepted`, `TradeExecuted`, etc.) são enviados para a `Execution Queue`.
    * Eventos de dados de mercado (`BookUpdateEvent`, etc.) são enviados para a `Market Data Channel`.

### 7. Execution Queue (Fila de Execução)

* **Responsabilidade Principal:** Servir como um buffer seguro para **eventos transacionais críticos** que não podem ser perdidos.
* **Entradas:** Objetos de "Evento" transacionais, vindos do `Event Bus / Dispatcher`.
* **Processamento:** Armazena os eventos em uma estrutura FIFO protegida por `mutex`.
* **Saídas:** Objetos de "Evento" para o `Outbound Gateway` e para o `Auditor`.

### 8. Market Data Channel (Canal de Dados de Mercado)

* **Responsabilidade Principal:** Servir como um canal de distribuição para **dados de mercado**, que são de alto volume e onde apenas o estado mais recente importa.
* **Entradas:** Objetos de "Evento" de dados de mercado, vindos do `Event Bus / Dispatcher`.
* **Processamento:** Implementado com o padrão de "Estado Compartilhado + Notificação" (`mutex` + `condition_variable`) para garantir que os consumidores sempre peguem o dado mais recente.
* **Saídas:** O último `MarketDataSnapshotEvent` para o `Market Data Gateway`.

### 9. Outbound Gateway

* **Responsabilidade Principal:** Comunicar os resultados **transacionais** de volta ao cliente (simulando FIX).
* **Entradas:** Objetos de "Evento" da `Execution Queue`.
* **Processamento:** Converte os eventos internos (`TradeExecuted`, etc.) em mensagens `ExecutionReport` no formato FIX.
* **Saídas:** Mensagens de resposta para o cliente.

### 10. Auditor (Journaler)

* **Responsabilidade Principal:** Criar um registro de auditoria completo dos eventos **transacionais**.
* **Entradas:** Objetos de "Evento" da `Execution Queue`.
* **Processamento:** Formata cada evento em uma linha de texto padronizada e a persiste em disco.
* **Saídas:** Um arquivo de texto (`SystemJournal.txt`).

### 11. Market Data Gateway

* **Responsabilidade Principal:** Distribuir os dados de mercado em tempo real para todos os clientes interessados.
* **Entradas:** O último `MarketDataSnapshotEvent` disponível no `Market Data Channel`.
* **Processamento:** Formata os dados do snapshot em um formato apropriado para broadcast (ex: JSON).
* **Saídas:** Um fluxo contínuo de dados de mercado para os clientes.
