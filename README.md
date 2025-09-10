# TagStream 🏷️➡️📊

**Um Projeto de IoT Aplicado à Metodologia 5S para Gestão de Ativos.**

---

## 👥 Equipe do Projeto

### Professores
* Andressa Schigatto Damori
* David Rafael de Freitas
* Everton Geisler
* Sávio Eduardo Zoboli

### Alunos
* Alexander
* Murillo Bento

---

Este projeto aplica os princípios da metodologia **5S** para transformar o gerenciamento de notebooks da instituição, utilizando tecnologia IoT para garantir um ambiente mais organizado, padronizado e eficiente.

* **Seiri (Utilização):** Separa notebooks funcionais dos que precisam de reparo, através de um status digital de "manutenção".
* **Seiton (Organização):** Define um "lugar" digital para cada equipamento, permitindo saber instantaneamente quem está com qual notebook.
* **Seisō (Limpeza):** Elimina a desordem do processo manual (papéis, anotações) e facilita a limpeza e manutenção programada dos equipamentos.
* **Seiketsu (Padronização):** Cria um processo eletrônico único e à prova de erros para empréstimo e devolução, válido para todos os usuários.
* **Shitsuke (Disciplina):** A própria tecnologia reforça a disciplina, pois o sistema automatizado garante que o padrão seja sempre seguido.

## 🎯 O Problema

O processo atual de empréstimo de notebooks é manual, suscetível a falhas, perdas de registro e depende da presença de um responsável. Não há rastreabilidade em tempo real do inventário, dificultando a organização e a programação de manutenções.

## 💡 A Solução

**TagStream** é uma plataforma de IoT que automatiza o fluxo de empréstimo e devolução. Utilizando tags RFID para autenticação, o sistema permite que os usuários realizem as operações de forma autônoma, enquanto todas as transações são registradas em um banco de dados central.

## 🛠️ Arquitetura da Solução

1.  **ESP32 (Hardware):** O terminal físico para interação do usuário, equipado com leitor RFID, teclado e display. Opera com resiliência, mesmo offline.

2.  **Node-RED (Backend/Lógica):** O cérebro do sistema, que recebe os dados do hardware (via MQTT), processa as regras de negócio e se comunica com o banco de dados.

3.  **PostgreSQL (Banco de Dados):** Armazena de forma persistente e segura os dados de usuários, notebooks e o histórico de todas as transações.

4.  **ReLab (Infraestrutura):** O ambiente de servidor (Docker, VPN) que hospeda os serviços de backend e o banco de dados do projeto.

## 📂 Estrutura do Repositório

* `/diagramas`: Arquitetura da solução e outros fluxos.
* `/firmware`: Código-fonte do firmware para o ESP32.
* `/database`: Scripts SQL para a criação do banco de dados.
* `/nodered-flow`: Arquivo de backup do fluxo do Node-RED.