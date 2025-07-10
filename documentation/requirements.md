## Requisitos Funcionais

### RF01: Receber e Rejeitar Ordens (Simulando Protocolo FIX)

O sistema deve ter um ponto de entrada para receber ordens de compra e venda. Ele também deve ser capaz de validar e rejeitar ordens que não atendam aos critérios básicos do negócio.

O **Gateway de Entrada (Input Gateway)** é o componente responsável por esta função. Ele irá ler uma representação simplificada de uma mensagem FIX (Por exemplo `NewOrderSingle`, que seria equivalente a 35=D).

* **Validação:** Ao receber uma ordem, o Gateway deve realizar verificações imediatas:
    * O símbolo do ativo é válido/conhecido?
    * O preço é um valor positivo? (Para `Limit Orders`).
    * A quantidade é um valor positivo?
    * O lado (`Side`) é 'Compra' ou 'Venda'?

* **Aceitação:** Se a ordem for válida, o Gateway a traduz para o nosso objeto `Order` interno e a coloca na **Fila de Entrada (Inbound Queue)** para ser processada pelo Motor.

* **Rejeição:** Se a ordem for inválida, o Gateway não a envia para o motor. Em vez disso, ele gera um evento de **Rejeição (Reject)** com o motivo da falha e o envia diretamente para a **Fila de Saída (Outbound Queue)**, para que o cliente seja notificado.

### RF02: Armazenar Ordens no Livro de Ofertas por Ativo

 As ordens que são aceitas, mas não executadas imediatamente, devem ser armazenadas de forma organizada em um "Livro de Ofertas" (`Order Book`), que é específico para cada ativo.

* O **Motor de Matching (Matching Engine)** é o dono dos livros de ofertas.
* Teremos uma estrutura de dados principal que mapeia um `Symbol` (ex: "PETR4") para o seu respectivo `OrderBook`.
* Cada `OrderBook` é, na verdade, composto por duas estruturas de dados separadas:
    * **Bids (Compras):** Onde as ordens de compra são armazenadas, ordenadas pela regra de prioridade (preço mais alto primeiro).
    * **Asks (Vendas):** Onde as ordens de venda são armazenadas, ordenadas pela regra de prioridade (preço mais baixo primeiro).
* Quando o motor processa uma nova ordem que não cruza com nenhuma outra, ele a insere na estrutura correta (Bid ou Ask) dentro do `Order Book` do seu ativo.

### RF03: Executar Ordens com Base no Preço e Tempo

Esta é a lógica central do sistema. O motor deve continuamente tentar cruzar ordens de compra e venda, respeitando a prioridade de preço e, para preços iguais, a prioridade de tempo (quem chegou primeiro).

* O **Motor de Matching**, ao receber uma nova ordem (a ordem "agressora"), verifica o lado oposto do livro.
* **Exemplo:** Chega uma ordem de compra de 100 ações de `PETR4` a R$ 30,00.
    * O motor olha o lado das vendas (`Asks`) do livro de `PETR4`.
    * Ele procura a ordem de venda com o menor preço. Digamos que seja uma venda a R$ 29,98.
    * Como o preço de compra (30,00) é maior ou igual ao preço de venda (29,98), um negócio pode acontecer!
    * O negócio é fechado pelo preço da ordem que já estava no livro (a ordem "passiva"), ou seja, R$ 29,98.
    * A quantidade executada será o mínimo entre as duas ordens.
* **Prioridade de Tempo:** Se houver múltiplas ordens de venda a R$ 29,98, o motor escolherá a que foi inserida no livro primeiro.

### RF04: Disparar Eventos de Execução Detalhados

Após qualquer ação significativa em uma ordem, o sistema deve gerar e disparar um evento claro e padronizado, informando o novo estado da ordem.

* O **Motor de Matching** é o responsável por gerar esses eventos após processar uma ordem.
* Os eventos são colocados na **Fila de Saída (Outbound Queue)** para serem consumidos pelo **Publicador de Eventos**.
* **Alguns exemplos de eventos (mais detalhes no `entities.md`):**
    * `Accepted`: A ordem foi aceita, mas ainda não executada (está passiva no livro).
    * `Canceled`: Uma ordem que estava no livro foi cancelada com sucesso.
    * `Partially Filled`: Um negócio ocorreu, mas a ordem foi apenas parcialmente executada. O evento deve informar a quantidade executada e a quantidade que ainda resta.
    * `Filled`: A ordem foi completamente executada por um ou mais negócios.
    * `Trade`: Um match entre duas ordens aconteceu e um trade foi executado.

### RF05: Armazenar um Journal (Log) de Todos os Eventos de Saída

O sistema deve manter um registro cronológico, sequencial e imutável de todos os eventos significativos que processa. Este journal é a fonte da verdade e é crucial para auditoria e recuperação.

* O **Publicador de Eventos** (ou um componente dedicado de `Journaling`) será responsável por esta tarefa.
* Toda vez que um evento é retirado da **Fila de Saída** (seja uma Rejeição, uma Aceitação, um `Fill`, etc.), ele deve ser escrito de forma serializada no arquivo de journal.
* O formato deve ser padronizado, por exemplo: `[Timestamp] [Tipo de Evento] [Detalhes do Evento em formato Chave=Valor]`
    ```log
    2025-07-09T19:18:46.123Z NEW_ORDER OrderID=123 Symbol=PETR4 Side=Buy Qty=100 Price=30.00
    2025-07-09T19:18:46.456Z TRADE TradeID=1 Price=29.98 Qty=50 AggressorID=123 PassiveID=120
    ```
* Este log é a prova de que tudo aconteceu e na ordem correta.

### RF06: Armazenar um Journal (Log) de Todos os Eventos de Entrada

O sistema deve manter um registro cronológico, sequencial e imutável de todos os eventos (FIX) significativos que processa (Write Ahead Logging). Este log é a fonte da verdade e é crucial para auditoria e recuperação.

* Um componente dedicado de `Journaling` será responsável por esta tarefa.
* Toda vez que um FIX é recebido no **Inbound Gateway**, ele deve ser escrito **primeiro** num arquivo de log e só depois o respectivo comando deve ser enviado para a **Inbound Queue**.
* Este log é a prova de que tudo aconteceu e na ordem correta. Em um sistema real, ele seria usado para reconstruir o estado do `Order Book` em caso de falha ou para permitir `Replays` caso necessário (release, investigação de bug, etc.).

### RF07: Permitir o Cancelamento de Ordens e Alteração de Ordens

Um cliente deve ser capaz de solicitar o cancelamento/alteração de uma ordem que ele enviou anteriormente e que ainda não foi totalmente executada.

* O **Gateway de Entrada** recebe uma solicitação (simulando uma mensagem FIX `OrderCancelRequest` 35=F ou `OrderAmendRequest` 35=G), que contém o `OrderID` da ordem a ser impactada.
* Essa solicitação é colocada na **Fila de Entrada**.
* O **Motor de Matching** processa a solicitação: ele busca a ordem pelo `OrderID` no seu respectivo `OrderBook`.
* Se um cancelamento foi solicitado:
   * Se a ordem for encontrada, ela é removida do livro. O motor então gera um evento `Cancel Accept` e o coloca na **Fila de Saída**.
   * Se a ordem não for encontrada (porque já foi totalmente executada ou o ID está errado), o motor pode gerar um evento de `Cancel Rejected`.
* Se uma alteração foi solicitada:
   * Se a ordem for encontrada, ela é removida do livro. O motor então modifica seus parâmetros, volta ela pro book (alterando sua prioridade), emite um evento de `Ammend Accept` e o coloca na **Fila de Saída**.
   * Se a ordem não for encontrada (porque já foi totalmente executada ou o ID está errado), o motor pode gerar um evento de `Ammend Rejected`.

### RF08: Publicação de Dados de Mercado (Market Data)

O sistema deve ser capaz de fornecer e publicar informações sobre o estado do livro de ofertas para os clientes. Isso inclui, no mínimo, o "Top of Book" (melhor preço de compra e venda) e, idealmente, a profundidade do mercado.

* **Top of Book (ToB):** A melhor oferta de compra (o maior preço) e a melhor oferta de venda (o menor preço) disponíveis.
* **Profundidade do Mercado (Market Depth):** Uma visão agregada do livro, mostrando a quantidade total de ações disponíveis em cada nível de preço. Ex: `PETR4 Bids: 1000 @ 30.00, 5000 @ 29.99, ...`
* **Impacto na Arquitetura:** O `Matching Engine`, após qualquer mudança no `OrderBook` (nova ordem, cancelamento, trade), geraria um snapshot do estado do mercado (ToB ou Depth). Esse snapshot seria um novo tipo de evento, colocado na **Fila de Saída** para ser distribuído pelo **Publicador de Eventos**, assim como os relatórios de execução.

### RF09: Suporte para Ordens a Mercado (Market Order)

O sistema deve aceitar "Ordens a Mercado", que são ordens de compra ou venda para serem executadas imediatamente ao melhor preço disponível no mercado. O cliente não especifica um preço, apenas a quantidade.

* Uma ordem a mercado é, por natureza, sempre "agressora". Ela nunca descansa no livro.
* **Lógica de Execução:** Ao receber uma ordem de compra a mercado, o **Motor de Matching** deve executá-la contra as ordens de venda (`Asks`) no livro, começando pela de menor preço e subindo até que a quantidade da ordem a mercado seja totalmente preenchida. O processo é o inverso para uma ordem de venda a mercado (começa executando contra o `Bid` de maior preço).
* **Impacto na Arquitetura:** A lógica no `Matching Engine` se torna um pouco mais complexa, pois precisa iterar por múltiplos níveis de preço do livro para satisfazer uma única ordem de entrada.

## Requisitos Não-Funcionais (RNFs)

### RNF01: Baixa Latência (Low Latency)

Representa o tempo total para processar uma única ordem, desde sua chegada no sistema até a geração de um resultado. Uma baixa latência é crucial para garantir que as negociações sejam justas e executadas ao melhor preço possível em um mercado que muda a cada instante.

* A latência de 99% de todas as ordens processadas (percentil 99) deve ser **inferior a 10 milissegundos (ms)**.

### RNF02: Alto Throughput (Vazão)

É a capacidade do sistema de processar um grande volume de ordens por segundo de forma contínua. Um alto throughput é essencial para suportar momentos de alta volatilidade do mercado sem criar filas de processamento e aumentar a latência para todos.

* O sistema deve suportar uma vazão sustentada de, no mínimo, **1.000 ordens por segundo**.

### RNF03: Concorrência (Concurrency)

É a capacidade do sistema de gerenciar múltiplas operações e requisições simultaneamente de forma segura, garantindo a consistência e a integridade dos dados, especialmente do livro de ofertas. A concorrência é a principal ferramenta para se alcançar alto throughput.

* O sistema deve passar em todos os testes de estresse que utilizam múltiplas threads para enviar ordens e cancelamentos concorrentemente, sem gerar *race conditions* (condições de corrida), *deadlocks* ou qualquer inconsistência no estado final do livro.

### RNF04: Resiliência e Correção (Resilience & Correctness)

Garante que o sistema é robusto contra falhas, se comporta de maneira previsível mesmo com entradas inesperadas e que todos os seus cálculos e transações são perfeitos e auditáveis. A correção é inegociável em um sistema financeiro.

* Taxa de Perda de Ordens ou Negócios: **0%**.
* Taxa de Rejeição de ordens por falha interna do sistema: **0%**.

### RNF05: Observabilidade (Observability)

É a capacidade de entender o estado e o comportamento interno do sistema através de suas saídas externas, como logs detalhados e métricas. Uma boa observabilidade é indispensável para depurar, monitorar e entender a performance do sistema.

* O `Journal` de eventos deve registrar 100% das transações do sistema (novas ordens, cancelamentos, negócios, etc.).
* O atraso entre um evento ocorrer e ser persistido no `Journal` deve ser **inferior a 1 segundo**.
