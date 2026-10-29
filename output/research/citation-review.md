# Citation review

The cited copy preserves the paper's wording and black formatting. Bracketed numbers identify sources in the bibliography. Citations placed within a sentence support the preceding clause, not every claim that follows. The ten original URLs are numbered in their existing order; [11]–[13] are specific articles reached from the existing HiveMQ source [8]. This is a simple numbered system, not a full conversion to a prescribed academic citation style.

## Claims that still need attention

- Introduction: the general statement that protocol selection affects efficiency and reliability is reasonable background, but the sources do not establish a measured comparison for this project.
- Client types: the sources support publisher/subscriber roles; these are not mutually exclusive device classes.
- Topics: distinguish a published topic name from the subscription filter. The cited HiveMQ article discusses both.
- QoS definition and introduction: the generic performance definition is not MQTT's delivery-assurance definition. QoS is not simply one setting for an entire connection. The three delivery-level descriptions match source [1], but delivery guarantees have scope and conditions; they do not prove that an actuator performed an action.
- No current subscribers: the statement about discarding all non-retained messages needs qualification for persistent sessions and offline subscribers. Source [1] itself discusses stored session messages. It was left uncited rather than presenting the simplification as universally valid.
- Reduced complexity: [8] supports compact packets and decoupled messaging. It does not establish that all HTTP versions impose heavy parsing or complex connection management.
- Decoupling: [8] supports the software explanation at the end, not the broad economic definition at the beginning.
- Polling: the claim that polling necessarily wastes resources needs conditions or comparative evidence; event-driven distribution alone does not quantify savings.
- Energy use: small headers alone do not prove lower radio-on time or total battery consumption in this greenhouse. The Keep Alive and QoS citations support their mechanisms, not a measured power advantage.
- Lightweight: [1] and [8] give an approximately 256 MB upper bound. Specify the MQTT version and distinguish packet size, Remaining Length, and payload before using a precise maximum. A maximum size does not establish that typical messages are small.
- Broker failure: [13] supports the single-broker failure scenario. Local device control and independent communications need not stop when that broker fails.
- Lack of scalability: [7] and [8] explicitly describe scaling MQTT to millions of devices. Reframe this as deployment and broker capacity constraints; a single point of failure is an availability issue, not proof of a protocol-wide inability to scale.
- TCP and QoS 0: the transport paragraph still says 'relies strictly on TCP' although the introduction gives the broader transport requirement. The following paragraph still claims QoS 0 overcomes the problem and mimics UDP. Source [1], Quality of service, says QoS does not change the underlying TCP transmissions. These paragraphs were not given blanket citations.
- Head-of-line blocking: the generic definition needs a directly supporting source if retained. A new source was not added solely to justify it.

## Comparison table

Citations are attached to supported cells, not to entire columns. Uncited cells are not automatically false, but require qualification or stronger evidence:

- MQTT overhead: 2 bytes is a minimum, not every fixed header or the full transmitted packet overhead.
- MQTT security: [7] supports TLS and authentication; 'TLS / SSL (handled by developer)' is too imprecise to cite as a current security recommendation.
- MQTT QoS: QoS 0 does not guarantee delivery, so the parenthetical needs qualification.
- HTTP transport: HTTP/3 is a protocol version, not a transport. The listed HTTP source explains its use of QUIC over UDP.
- HTTP overhead: the listed HTTP source distinguishes text-based HTTP/1.x from binary, compressed HTTP/2 and HTTP/3. Cookies are not required in every request.
- HTTP security: HTTPS is supported, but the table's restriction to TLS 1.3 is not established for HTTP generally.
- HTTP reliability: 'relies on TCP' excludes HTTP/3.
- Best Used For: the cited cells describe supported use cases; they do not prove that the protocol is the best option in every such case. The HTTP cell's general ranking and 'heavy data transfers' characterization remain unsupported.

## Greenhouse conclusion and images

The conclusion is your design judgement. None of these sources can establish that MQTT is best for your greenhouse without explicit requirements and a comparison against them. The claims about high speed, power cost, HTTP transfers, and CoAP being unsuitable still need evidence or qualification. I have not added a citation that would falsely imply the conclusion has been demonstrated.

The three existing diagrams still need confirmed original image credits. A source supporting the subject of a diagram is not necessarily the source of that image, so image attribution was not guessed.

Some definitions closely match source wording, especially in [1] and [11]. Adding a citation attributes the source but does not turn verbatim text into a paraphrase. Before submission, quote those passages or rewrite them in your own words according to your course's rules.

## Citation placement record

- over a network. → [8]
- MQTT is a lightweight, publish–subscribe, machine-to-machine network protocol for message queue → [1]
- The MQTT protocol defines two types of network entities: a message broker and a number of clien → [1]
- 2. Client - any device (from a microcontroller up to a full-fledged server) that runs an MQTT l → [1, 8]
- N.B! A client can be both a publisher and subscriber! → [1]
- 4. Subscriber - a client device or application that connects to an MQTT broker and listens for  → [8]
- Information is organized in a hierarchy of topics. When a publisher has a new item of data to d → [1]
- N.B! - In comparison to a message queue, MQTT topics are very lightweight. The client does not  → [11]
- 6.1. At most once – the message is sent only once and the client and broker take no additional  → [1]
- 6.2. At least once – the message is re-tried by the sender multiple times until acknowledgement → [1]
- 6.3. Exactly once – the sender and receiver engage in a two-level handshake to ensure only one  → [1]
- 7. Retained messages - A retained message is a normal MQTT message with the retained flag set t → [8]
- 8.1. The Internet protocol suite, commonly known as TCP/IP, is a framework for organizing the c → [2, 6]
- 8.2. The Transmission Control Protocol (TCP) provides reliable, ordered, and error-checked deli → [3]
- 8-byte header. → [4]
- 8.4. The Internet Protocol (IP) is the foundational set of rules that governs how data packets  → [5]
- Publish/Subscribed architecture → [8]
- *decoupling - separating two or more systems, activities, or economic factors so that they no l → [8]
- MQTT eliminates polling by using a push-based pub/sub architecture in which the client establis → [8]
- over long periods → [12]
- delivery-acknowledgement handshakes → [8]
- *Keep Alive is a feature of the MQTT protocol that allows an MQTT client to maintain its connec → [12]
- The protocol overhead of MQTT is extremely small and the protocol was overall designed to send  → [1, 8]
- The MQTT protocols uses centralized routing. This means that all publishers and subscirbers rou → [13]
- Disadvantage 1 and 2 can be overcomed by the concept of clustering. → [13]
- MQTT clustering means grouping connected MQTT brokers that work together as a single, unified m → [13]
- Publish / Subscribe (via Broker) → [8]
- Request / Response (Client-Server) → [9]
- Request / Response (REST-like, Peer-to-Peer capable) → [10]
- TCP (persistent connection) → [1]
- UDP (with confirmable/non-confirmable options) → [10]
- Very low (binary header, small size) → [10]
- DTLS (Datagram TLS) → [10]
- Confirmable vs. Non-confirmable messages → [10]
- Real-time sensor telemetry, pub-sub messaging, unstable/cellular networks → [7]
- Extremely constrained microcontrollers, low-power sensor mesh networks → [10]
