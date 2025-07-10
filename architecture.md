## Descrição dos Componentes da Arquitetura
Esta documentação descreve as responsabilidades e interações de cada componente principal do Simulador de Livro de Ofertas.

![architecture](https://github.com/user-attachments/assets/8ae2028a-331e-4e9e-9700-16ba3eb465e9)

### 1. Inbound Gateway

* **Responsabilidade Principal:** Servir como a porta de entrada do sistema. É responsável por receber, registrar para segurança e traduzir as requisições dos clientes em comandos internos.
* **Entradas:** Comandos brutos em formato de texto (simulando o protocolo FIX), como `NEW_ORDER` ou `CANCEL_ORDER`.
* **Processamento:**
    1.  **Registro (Write-Ahead Log):** Imediatamente escreve uma cópia exata do comando bruto recebido no `Input Log`.
    2.  **Interpretação:** Analisa o comando para entender a intenção do cliente.
    3.  **Criação de Comando (Fábrica):** Com base na intenção, instancia o objeto de "Comando" apropriado (ex: `NewOrderCommand` ou `CancelOrderCommand`), preenchendo-o com os dados da requisição.
* **Saídas:** Um "objeto de comando" que é colocado na `Thread Safe Inbound Queue`.

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
    1.  Retira um `Comando` da fila.
    2.  Delega a execução para o próprio objeto de comando, que invoca a lógica apropriada no motor.
    3.  Toda a interação com o `Order Book` (consultas, inserções, remoções) é realizada aqui.
    4.  Ao final de cada operação, gera um ou mais objetos de "Evento" (`OrderAccepted`, `TradeExecuted`, etc.) para descrever o resultado.
* **Saídas:** Objetos de "Evento" que são colocados na `Thread Safe Outbound Queue`.

### 5. Order Book

* **Responsabilidade Principal:** Manter o estado atual e ordenado de todas as ordens ativas de compra (Bids) e venda (Asks). É o "banco de dados em memória" do `Matching Engine`.
* **Entradas:** Requisições de inserção, remoção e consulta vindas do `Matching Engine`.
* **Processamento:** Organiza as ordens seguindo a regra de prioridade Preço-Tempo.
* **Saídas:** Respostas às consultas do `Matching Engine`.

### 6. Thread Safe Outbound Queue

* **Responsabilidade Principal:** Desacoplar o `Matching Engine` dos sistemas de saída, criando um "rio de eventos" centralizado que serve como a única fonte da verdade sobre o que aconteceu no sistema.
* **Entradas:** Objetos de "Evento" (`OrderAccepted`, `TradeExecuted`, etc.) gerados pelo `Matching Engine`.
* **Processamento:** Armazena os eventos em uma estrutura FIFO protegida por um `mutex`.
* **Saídas:** Objetos de "Evento" para todos os seus consumidores (`Outbound Gateway` e `Auditor`).

### 7. Outbound Gateway

* **Responsabilidade Principal:** Comunicar os resultados do processamento de volta ao cliente final, no formato de protocolo esperado (simulando FIX).
* **Entradas:** Objetos de "Evento" da `Thread Safe Outbound Queue`.
* **Processamento:** Filtra os eventos relevantes para o cliente. Converte os dados do objeto de evento interno em uma mensagem de resposta no formato FIX (`ExecutionReport`).
* **Saídas:** Mensagens de resposta para o cliente.

### 8. Auditor (Journaler)

* **Responsabilidade Principal:** Criar um registro de auditoria completo, persistente e legível por humanos de todos os eventos significativos que ocorreram no sistema.
* **Entradas:** Objetos de "Evento" da `Thread Safe Outbound Queue`.
* **Processamento:** Consome todos os eventos da fila e os formata em uma representação de texto padronizada.
* **Saídas:** Um arquivo de texto (`SystemJournal.txt`) persistido em disco.
