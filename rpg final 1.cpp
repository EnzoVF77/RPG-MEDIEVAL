#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

// RPG Medieval - Tabela Hash (itens) + Grafo (mapa)

const int TAM = 30;  // tamanho da tabela hash
const int NUM_LOCAIS = 6;
const int VILA = 0, CASTELO = 5;
const int DISTANCIA_INICIAL = 5, DISTANCIA_MAXIMA = 8;

int sorteio(int a, int b) { return a + rand() % (b - a + 1); }
bool chance(int porcento) { return sorteio(1, 100) <= porcento; }

// ===== TABELA HASH: itens (sondagem linear para colisoes) =====

struct Item {
    string nome, tipo;  // tipo: "arma", "armadura", "pocao" ou "magia"
    int ataque;         // dano da arma ou da magia
    int defesa, cura, preco, alcance;
    int mana;           // custo de mana (magias)
    int quantidade;     // quantos o jogador tem (magia: 1 = aprendida)
    bool ocupado;
};

Item tabela[TAM];
bool catalogoFechado = false;  // depois de montarJogo(), ninguem cria itens

int calcularHash(string nome) {
    int soma = 0;
    for (int i = 0; i < (int)nome.size(); i++) {
        soma += (unsigned char)nome[i] * (i + 1);
    }
    return soma % TAM;
}

// a ordem dos parametros segue a ordem dos campos de Item
void inserirItem(string nome, string tipo, int ataque, int defesa, int cura,
                 int preco, int alcance, int mana, int quantidade) {
    if (catalogoFechado) { return; }
    int pos = calcularHash(nome);
    for (int t = 0; t < TAM && tabela[pos].ocupado; t++) { pos = (pos + 1) % TAM; }
    if (tabela[pos].ocupado) { return; }  // tabela cheia
    tabela[pos] = Item{nome, tipo, ataque, defesa, cura, preco, alcance, mana, quantidade, true};
}

// retorna a posicao do item na tabela, ou -1 se nao existir
int buscarItem(string nome) {
    int pos = calcularHash(nome);
    for (int t = 0; t < TAM && tabela[pos].ocupado; t++) {
        if (tabela[pos].nome == nome) { return pos; }
        pos = (pos + 1) % TAM;
    }
    return -1;
}

bool magiaAprendida(const Item &it) {
    return it.ocupado && it.tipo == "magia" && it.quantidade > 0;
}

string detalhes(const Item &it) {
    if (it.tipo == "arma") { return "ataque " + to_string(it.ataque) + ", alcance " + to_string(it.alcance); }
    if (it.tipo == "armadura") { return "defesa " + to_string(it.defesa); }
    if (it.tipo == "pocao") { return "cura " + to_string(it.cura); }
    return "dano " + to_string(it.ataque) + ", mana " + to_string(it.mana) + ", alcance " + to_string(it.alcance);
}

// ===== GRAFO: mapa (matriz de adjacencia) + inimigo de cada local =====

string nomeLocal[NUM_LOCAIS] = {"Vila de Eldoria", "Floresta Sombria", "Ruinas Antigas",
                                "Pantano Maldito", "Montanha Gelada", "Castelo Sombrio"};

struct Inimigo {
    string nome;
    int vida, ataque, alcance, ouroMin, ouroMax;
    string drop[2];  // itens que podem cair
    int chance[2];   // chance (%) de cada drop
};

Inimigo inimigo[NUM_LOCAIS] = {
    {"", 0, 0, 0, 0, 0, {"", ""}, {0, 0}},
    {"Lobo Selvagem", 30, 7, 1, 5, 15, {"Pocao de Cura", ""}, {50, 0}},
    {"Esqueleto Guerreiro", 40, 9, 1, 15, 30, {"Armadura Sombria", "Pocao de Cura"}, {25, 40}},
    {"Bruxa do Pantano", 45, 11, 4, 20, 35, {"Elixir Maior", "Fica frio"}, {40, 25}},
    {"Troll da Montanha", 70, 13, 2, 30, 55, {"Machado Rhitta", "Elixir Maior"}, {10, 30}},
    {"Lorde Sombrio", 110, 17, 3, 80, 135, {"Elixir Maior", ""}, {50, 0}}
};

bool derrotado[NUM_LOCAIS];
int caminho[NUM_LOCAIS][NUM_LOCAIS];  // 1 = existe estrada entre os locais

void ligarLocais(int a, int b) { caminho[a][b] = caminho[b][a] = 1; }

// So anda entre dois locais se o inimigo de um deles ja foi derrotado
bool caminhoLiberado(int a, int b) { return derrotado[a] || derrotado[b]; }

// Busca em largura (BFS): mostra o menor caminho entre dois locais
void mostrarRota(int origem, int destino) {
    int fila[NUM_LOCAIS], pai[NUM_LOCAIS], inicio = 0, fim = 0;
    bool visitado[NUM_LOCAIS];
    for (int i = 0; i < NUM_LOCAIS; i++) { visitado[i] = false; pai[i] = -1; }
    fila[fim++] = origem;
    visitado[origem] = true;

    while (inicio < fim) {
        int atual = fila[inicio++];
        for (int i = 0; i < NUM_LOCAIS; i++) {
            if (caminho[atual][i] == 1 && !visitado[i]) {
                visitado[i] = true;
                pai[i] = atual;
                fila[fim++] = i;
            }
        }
    }

    int rota[NUM_LOCAIS], tamanho = 0;  // monta a rota de tras para frente
    for (int v = destino; v != -1; v = pai[v]) { rota[tamanho++] = v; }
    cout << "\nMenor rota ate o castelo: ";
    for (int i = tamanho - 1; i >= 0; i--) { cout << nomeLocal[rota[i]] << (i > 0 ? " -> " : "\n"); }
}

// ===== JOGADOR =====

int vida = 50, vidaMaxima = 50, mana = 30, manaMaxima = 30, ouro = 20;
int localAtual = VILA;
string armaEquipada = "Adagas de Baruka", armaduraEquipada = "";
bool jogoAcabou = false, venceu = false;

int lerNumero() {
    int n;
    if (!(cin >> n)) {
        if (cin.eof()) { exit(0); }
        cin.clear();
        cin.ignore(1000, '\n');
        return -1;
    }
    return n;
}

string lerNome() {
    string nome;
    getline(cin >> ws, nome);
    return nome;
}

int ataqueTotal() { return 4 + tabela[buscarItem(armaEquipada)].ataque; }
int alcanceArma() { return tabela[buscarItem(armaEquipada)].alcance; }
int defesaTotal() { return armaduraEquipada == "" ? 0 : tabela[buscarItem(armaduraEquipada)].defesa; }

void mostrarStatus() {
    cout << "\n[Vida: " << vida << "/" << vidaMaxima << " | Mana: " << mana << "/" << manaMaxima
         << " | Ouro: " << ouro << " | Ataque: " << ataqueTotal() << " (alcance " << alcanceArma()
         << ") | Defesa: " << defesaTotal() << "]\n";
}

void mostrarInventario() {
    cout << "\n--- Inventario ---\n";
    for (int i = 0; i < TAM; i++) {
        const Item &it = tabela[i];
        if (it.ocupado && it.quantidade > 0 && it.tipo != "magia") {
            cout << " - " << it.nome << " x" << it.quantidade << " (" << it.tipo << ", " << detalhes(it) << ")\n";
        }
    }
    cout << "--- Magias ---\n";
    for (int i = 0; i < TAM; i++) {
        if (magiaAprendida(tabela[i])) { cout << " - " << tabela[i].nome << " (" << detalhes(tabela[i]) << ")\n"; }
    }
    cout << "Arma: " << armaEquipada << " | Armadura: "
         << (armaduraEquipada == "" ? "nenhuma" : armaduraEquipada) << "\n";
}

void equipar() {
    cout << "Nome do item: ";
    int pos = buscarItem(lerNome());
    if (pos == -1 || tabela[pos].quantidade == 0) {
        cout << "Voce nao tem esse item.\n";
    } else if (tabela[pos].tipo == "arma") {
        armaEquipada = tabela[pos].nome;
        cout << "Arma equipada!\n";
    } else if (tabela[pos].tipo == "armadura") {
        armaduraEquipada = tabela[pos].nome;
        cout << "Armadura equipada!\n";
    } else {
        cout << "Esse item nao pode ser equipado.\n";
    }
}

// retorna true se conseguiu usar a pocao
bool usarPocao() {
    cout << "Nome da pocao: ";
    int pos = buscarItem(lerNome());
    if (pos == -1 || tabela[pos].quantidade == 0 || tabela[pos].tipo != "pocao") {
        cout << "Voce nao pode usar isso.\n";
        return false;
    }
    tabela[pos].quantidade--;
    vida = min(vidaMaxima, vida + tabela[pos].cura);
    cout << "Vida agora: " << vida << "/" << vidaMaxima << "\n";
    return true;
}

void loja() {
    if (localAtual != VILA) { cout << "A loja fica na Vila de Eldoria.\n"; return; }
    const int N = 8;
    const string produtos[N] = {"Pocao de Cura", "Elixir Maior", "Espada de Ferro", "Espada de Aco",
                                "Arco Longo", "Armadura de Couro", "Cota de Malha", "Bola de Fogo"};
    int opcao = -1;
    while (opcao != 0) {
        cout << "\n--- Loja (ouro: " << ouro << ") ---\n";
        for (int i = 0; i < N; i++) {
            const Item &it = tabela[buscarItem(produtos[i])];
            cout << " " << i + 1 << ") " << it.nome << " - " << it.preco << " ouro";
            if (it.tipo == "arma" || it.tipo == "magia") { cout << " [" << detalhes(it) << "]"; }
            cout << "\n";
        }
        cout << " 0) Sair\n> ";

        opcao = lerNumero();
        if (opcao < 1 || opcao > N) { continue; }
        Item &it = tabela[buscarItem(produtos[opcao - 1])];
        if (it.tipo != "pocao" && it.quantidade > 0) {
            cout << "Voce ja possui isso.\n";  // equipamentos e magias sao unicos
        } else if (ouro < it.preco) {
            cout << "Ouro insuficiente!\n";
        } else {
            ouro -= it.preco;
            it.quantidade++;
            cout << "Voce comprou " << it.nome << "!\n";
        }
    }
}

void olhar() {
    const Inimigo &e = inimigo[localAtual];
    cout << "\n=== " << nomeLocal[localAtual] << " ===\n";
    if (e.nome == "") { return; }
    if (!derrotado[localAtual]) { cout << "Perigo! " << e.nome << " esta aqui!\n"; }
    else { cout << "A regiao esta calma, mas criaturas ainda rondam (voce pode cacar).\n"; }
}

void mover() {
    cout << "\nPara onde deseja ir?\n";
    for (int i = 0; i < NUM_LOCAIS; i++) {
        if (caminho[localAtual][i] == 0) { continue; }
        cout << " " << i << ") " << nomeLocal[i];
        if (!caminhoLiberado(localAtual, i)) { cout << " [BLOQUEADO - derrote " << inimigo[localAtual].nome << "]"; }
        cout << "\n";
    }
    cout << "> ";

    int destino = lerNumero();
    if (destino < 0 || destino >= NUM_LOCAIS || caminho[localAtual][destino] == 0) {
        cout << "Destino invalido.\n";
    } else if (!caminhoLiberado(localAtual, destino)) {
        cout << "Caminho bloqueado! Derrote " << inimigo[localAtual].nome << " primeiro.\n";
    } else {
        localAtual = destino;
        olhar();
        if (localAtual == VILA) {
            vida = vidaMaxima;
            mana = manaMaxima;
            cout << "Voce descansa na estalagem e recupera vida e mana.\n";
        }
    }
}

// ===== COMBATE (distancia, alcance de armas e magias) =====

void recompensas(const Inimigo &e) {
    int ganho = sorteio(e.ouroMin, e.ouroMax);
    ouro += ganho;
    cout << "\nVoce venceu " << e.nome << "! +" << ganho << " de ouro.\n";

    for (int d = 0; d < 2; d++) {
        if (e.drop[d] == "" || !chance(e.chance[d])) { continue; }
        int pos = buscarItem(e.drop[d]);
        if (pos == -1) { continue; }  // so existem itens ja cadastrados
        if (tabela[pos].tipo != "magia") {
            tabela[pos].quantidade++;
            cout << "Item obtido: " << tabela[pos].nome << "\n";
        } else if (tabela[pos].quantidade == 0) {
            tabela[pos].quantidade = 1;
            cout << "Voce encontrou um grimorio e aprendeu: " << tabela[pos].nome << "!\n";
        }
    }
}

// lista as magias aprendidas; retorna a posicao na tabela da escolhida, ou -1
int escolherMagia() {
    int posicoes[TAM], n = 0;
    cout << "\n--- Magias ---\n";
    for (int i = 0; i < TAM; i++) {
        if (!magiaAprendida(tabela[i])) { continue; }
        posicoes[n++] = i;
        cout << " " << n << ") " << tabela[i].nome << " (" << detalhes(tabela[i]) << ")\n";
    }
    cout << " 0) Voltar\n> ";
    int op = lerNumero();
    return (op < 1 || op > n) ? -1 : posicoes[op - 1];
}

// as tres funcoes abaixo retornam true se a acao gastou o turno
bool atacarComArma(int distancia, int &vidaInimigo) {
    if (distancia > alcanceArma()) { cout << "Inimigo fora de alcance!\n"; return false; }
    int dano = ataqueTotal() + sorteio(0, 2);
    vidaInimigo -= dano;
    cout << "Voce causou " << dano << " de dano.\n";
    return true;
}

bool lancarMagia(int distancia, int &vidaInimigo) {
    int pos = escolherMagia();
    if (pos == -1) { return false; }
    if (mana < tabela[pos].mana) { cout << "Mana insuficiente!\n"; return false; }
    if (distancia > tabela[pos].alcance) { cout << "Fora do alcance da magia!\n"; return false; }
    mana -= tabela[pos].mana;
    int dano = tabela[pos].ataque + sorteio(0, 3);  // magia ignora armadura
    vidaInimigo -= dano;
    cout << "Voce lancou " << tabela[pos].nome << " e causou " << dano << " de dano.\n";
    return true;
}

bool mudarDistancia(int &distancia, int passo) {  // passo -1 aproxima, +1 recua
    if (passo < 0 && distancia <= 1) { cout << "Voce ja esta colado no inimigo.\n"; return false; }
    if (passo > 0 && distancia >= DISTANCIA_MAXIMA) { cout << "Nao ha espaco para recuar.\n"; return false; }
    distancia += passo;
    cout << (passo < 0 ? "Voce se aproxima" : "Voce recua") << ". Distancia: " << distancia << "\n";
    return true;
}

void turnoInimigo(const Inimigo &e, int &distancia) {
    if (distancia <= e.alcance) {
        int dano = max(1, e.ataque + sorteio(-1, 1) - defesaTotal());
        vida -= dano;
        cout << e.nome << " causou " << dano << " de dano em voce.\n";
    } else {
        distancia--;
        cout << e.nome << " avanca. Distancia: " << distancia << "\n";
    }
}

void combate() {
    const Inimigo &e = inimigo[localAtual];
    if (e.nome == "") { cout << "Nao ha inimigos aqui.\n"; return; }

    bool cacada = derrotado[localAtual];  // luta repetida: so da recompensas
    int vidaInimigo = e.vida, distancia = DISTANCIA_INICIAL;
    cout << "\n*** Combate contra " << e.nome << " ***\n";
    if (cacada) { cout << "(Uma nova criatura aparece.)\n"; }

    while (vida > 0 && vidaInimigo > 0) {
        cout << "\nSua vida: " << vida << " | Mana: " << mana << " | Inimigo: " << vidaInimigo
             << " | Distancia: " << distancia << " (seu alcance " << alcanceArma()
             << ", inimigo " << e.alcance << ")\n"
             << "1) Atacar  2) Magia  3) Pocao  4) Aproximar  5) Recuar  6) Fugir\n> ";

        bool gastouTurno = false;
        switch (lerNumero()) {
            case 1: gastouTurno = atacarComArma(distancia, vidaInimigo); break;
            case 2: gastouTurno = lancarMagia(distancia, vidaInimigo); break;
            case 3: gastouTurno = usarPocao(); break;
            case 4: gastouTurno = mudarDistancia(distancia, -1); break;
            case 5: gastouTurno = mudarDistancia(distancia, +1); break;
            case 6: cout << "Voce fugiu!\n"; return;
        }
        if (!gastouTurno) { continue; }

        if (vidaInimigo > 0) { turnoInimigo(e, distancia); }
        mana = min(manaMaxima, mana + 2);
    }

    if (vida <= 0) {
        cout << "\nVoce foi derrotado... Fim de jogo.\n";
        jogoAcabou = true;
        return;
    }
    recompensas(e);
    if (!cacada) { derrotado[localAtual] = true; }  // libera os caminhos
    if (localAtual == CASTELO) { venceu = true; }
}

// ===== MONTAGEM DO MUNDO E LOOP PRINCIPAL =====

void montarJogo() {
    //          nome                 tipo        atq def cura preco alc mana qtd
    inserirItem("Adagas de Baruka",  "arma",      1,  0,  0,   0,   1,  0,   1);
    inserirItem("Espada de Ferro",   "arma",      5,  0,  0,   30,  1,  0,   0);
    inserirItem("Espada de Aco",     "arma",      9,  0,  0,   70,  2,  0,   0);
    inserirItem("Arco Longo",        "arma",      6,  0,  0,   50,  5,  0,   0);
    inserirItem("Machado Rhitta",    "arma",      13, 0,  0,   0,   2,  0,   0);
    inserirItem("Armadura de Couro", "armadura",  0,  2,  0,   25,  0,  0,   0);
    inserirItem("Cota de Malha",     "armadura",  0,  5,  0,   60,  0,  0,   0);
    inserirItem("Armadura Sombria",  "armadura",  0,  8,  0,   0,   0,  0,   0);
    inserirItem("Pocao de Cura",     "pocao",     0,  0,  30,  10,  0,  0,   2);
    inserirItem("Elixir Maior",      "pocao",     0,  0,  60,  25,  0,  0,   0);
    inserirItem("Faisca",            "magia",     8,  0,  0,   0,   4,  5,   1);  // atq = dano
    inserirItem("Bola de Fogo",      "magia",     18, 0,  0,   80,  6,  12,  0);
    inserirItem("Fica frio",         "magia",     14, 0,  0,   0,   5,  9,   0);
    catalogoFechado = true;  // nenhum item novo pode ser criado daqui em diante

    // locais sem inimigo ja contam como "derrotados"
    for (int i = 0; i < NUM_LOCAIS; i++) { derrotado[i] = (inimigo[i].nome == ""); }

    ligarLocais(0, 1);  // Vila - Floresta
    ligarLocais(1, 2);  // Floresta - Ruinas
    ligarLocais(1, 3);  // Floresta - Pantano
    ligarLocais(2, 4);  // Ruinas - Montanha
    ligarLocais(3, 4);  // Pantano - Montanha
    ligarLocais(4, 5);  // Montanha - Castelo
}

int main() {
    srand((unsigned)time(0));
    montarJogo();
    cout << "=== A QUEDA DE ELDORIA ===\nDerrote o Lorde Sombrio no Castelo Sombrio!\n";
    olhar();

    while (!jogoAcabou && !venceu) {
        mostrarStatus();
        cout << "\n1) Olhar  2) Mover  3) Lutar  4) Inventario  5) Equipar\n"
             << "6) Usar pocao  7) Loja  8) Rota ate o castelo  0) Sair\n> ";

        int opcao = lerNumero();
        if (opcao == 0) { break; }
        switch (opcao) {
            case 1: olhar(); break;
            case 2: mover(); break;
            case 3: combate(); break;
            case 4: mostrarInventario(); break;
            case 5: equipar(); break;
            case 6: usarPocao(); break;
            case 7: loja(); break;
            case 8: mostrarRota(localAtual, CASTELO); break;
            default: cout << "Opcao invalida.\n";
        }
    }
    if (venceu) { cout << "\n*** VITORIA! Eldoria esta salva! ***\n"; }
    return 0;
}
