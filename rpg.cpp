#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

// RPG Medieval - Tabela Hash (itens) + Grafo (mapa)

int sorteio(int a, int b) { return a + rand() % (b - a + 1); }
bool chance(int pct) { return sorteio(1, 100) <= pct; }

// ============================================================
//  TABELA HASH: itens (sondagem linear para colisoes)
// ============================================================
const int TAM = 30;

struct Item {
    string nome, tipo;  // tipo: "arma", "armadura", "pocao" ou "magia"
    int ataque, defesa, cura, preco, alcance, mana;
    int quantidade;     // quantos o jogador tem (magia: 1 = aprendida)
    bool ocupado;
};

Item tabela[TAM];
bool catalogoFechado = false;  // depois de montarJogo(), ninguem cria itens

int calcularHash(string nome) {
    int soma = 0;
    for (int i = 0; i < (int)nome.size(); i++) soma += (unsigned char)nome[i] * (i + 1);
    return soma % TAM;
}

void inserirItem(string nome, string tipo, int atq, int def, int cura,
                 int preco, int alc, int mana, int qtd) {
    if (catalogoFechado) return;
    int pos = calcularHash(nome), tentativas = 0;
    while (tabela[pos].ocupado && tentativas++ < TAM) pos = (pos + 1) % TAM;
    if (tentativas > TAM) return;  // tabela cheia
    tabela[pos].nome = nome;     tabela[pos].tipo = tipo;
    tabela[pos].ataque = atq;    tabela[pos].defesa = def;
    tabela[pos].cura = cura;     tabela[pos].preco = preco;
    tabela[pos].alcance = alc;   tabela[pos].mana = mana;
    tabela[pos].quantidade = qtd; tabela[pos].ocupado = true;
}

int buscarItem(string nome) {  // posicao ou -1
    int pos = calcularHash(nome), tentativas = 0;
    while (tabela[pos].ocupado && tentativas++ < TAM) {
        if (tabela[pos].nome == nome) return pos;
        pos = (pos + 1) % TAM;
    }
    return -1;
}

// ============================================================
//  GRAFO: mapa (matriz de adjacencia) + inimigos de cada local
// ============================================================
const int NUM_LOCAIS = 6;
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
    {"Lobo Selvagem",      30,  7, 1,  5,  15, {"Pocao de Cura", ""},            {50, 0}},
    {"Esqueleto Guerreiro", 40,  9, 1, 15,  30, {"Armadura Sombria", "Pocao de Cura"}, {25, 40}},
    {"Bruxa do Pantano",   45, 11, 4, 20,  35, {"Elixir Maior", "Raio Gelido"},  {40, 25}},
    {"Troll da Montanha",  70, 13, 2, 30,  55, {"Machado do Troll", "Elixir Maior"}, {10, 30}},
    {"Lorde Sombrio",     110, 17, 3, 80, 135, {"Elixir Maior", ""},             {50, 0}}
};

bool derrotado[NUM_LOCAIS];
int caminho[NUM_LOCAIS][NUM_LOCAIS];

void ligarLocais(int a, int b) { caminho[a][b] = caminho[b][a] = 1; }

// So anda se o inimigo de um dos dois locais ja foi derrotado
bool caminhoLiberado(int a, int b) { return derrotado[a] || derrotado[b]; }

// BFS: menor caminho entre dois locais
void mostrarRota(int origem, int destino) {
    int fila[NUM_LOCAIS], pai[NUM_LOCAIS], inicio = 0, fim = 0;
    bool visitado[NUM_LOCAIS];
    for (int i = 0; i < NUM_LOCAIS; i++) { visitado[i] = false; pai[i] = -1; }
    fila[fim++] = origem;
    visitado[origem] = true;
    while (inicio < fim) {
        int atual = fila[inicio++];
        for (int i = 0; i < NUM_LOCAIS; i++)
            if (caminho[atual][i] && !visitado[i]) {
                visitado[i] = true;
                pai[i] = atual;
                fila[fim++] = i;
            }
    }
    int rota[NUM_LOCAIS], n = 0;
    for (int v = destino; v != -1; v = pai[v]) rota[n++] = v;
    cout << "\nMenor rota ate o castelo: ";
    for (int i = n - 1; i >= 0; i--) cout << nomeLocal[rota[i]] << (i > 0 ? " -> " : "\n");
}

// ============================================================
//  JOGADOR
// ============================================================
int vida = 50, vidaMaxima = 50, ouro = 20, mana = 30, manaMaxima = 30;
int localAtual = 0;
string armaEquipada = "Adaga Enferrujada", armaduraEquipada = "";
bool jogoAcabou = false, venceu = false;

int lerNumero() {
    int n;
    if (!(cin >> n)) {
        if (cin.eof()) exit(0);
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
        Item &it = tabela[i];
        if (!it.ocupado || it.quantidade == 0 || it.tipo == "magia") continue;
        cout << " - " << it.nome << " x" << it.quantidade << " (" << it.tipo;
        if (it.tipo == "arma") cout << ", ataque " << it.ataque << ", alcance " << it.alcance;
        if (it.tipo == "armadura") cout << ", defesa " << it.defesa;
        if (it.tipo == "pocao") cout << ", cura " << it.cura;
        cout << ")\n";
    }
    cout << "--- Magias ---\n";
    for (int i = 0; i < TAM; i++)
        if (tabela[i].ocupado && tabela[i].tipo == "magia" && tabela[i].quantidade > 0)
            cout << " - " << tabela[i].nome << " (dano " << tabela[i].ataque << ", mana "
                 << tabela[i].mana << ", alcance " << tabela[i].alcance << ")\n";
    cout << "Arma: " << armaEquipada << " | Armadura: "
         << (armaduraEquipada == "" ? "nenhuma" : armaduraEquipada) << "\n";
}

void equipar() {
    cout << "Nome do item: ";
    string nome = lerNome();
    int pos = buscarItem(nome);
    if (pos == -1 || tabela[pos].quantidade == 0) cout << "Voce nao tem esse item.\n";
    else if (tabela[pos].tipo == "arma") { armaEquipada = tabela[pos].nome; cout << "Arma equipada!\n"; }
    else if (tabela[pos].tipo == "armadura") { armaduraEquipada = tabela[pos].nome; cout << "Armadura equipada!\n"; }
    else cout << "Esse item nao pode ser equipado.\n";
}

bool usarPocao() {  // true se conseguiu usar
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
    if (localAtual != 0) { cout << "A loja fica na Vila de Eldoria.\n"; return; }
    const int N = 8;
    string produtos[N] = {"Pocao de Cura", "Elixir Maior", "Espada de Ferro", "Espada de Aco",
                          "Arco Longo", "Armadura de Couro", "Cota de Malha", "Bola de Fogo"};
    int opcao = -1;
    while (opcao != 0) {
        cout << "\n--- Loja (ouro: " << ouro << ") ---\n";
        for (int i = 0; i < N; i++) {
            Item &it = tabela[buscarItem(produtos[i])];
            cout << " " << i + 1 << ") " << it.nome << " - " << it.preco << " ouro";
            if (it.tipo == "arma") cout << " [ataque " << it.ataque << ", alcance " << it.alcance << "]";
            if (it.tipo == "magia") cout << " [dano " << it.ataque << ", mana " << it.mana << ", alcance " << it.alcance << "]";
            cout << "\n";
        }
        cout << " 0) Sair\n> ";
        opcao = lerNumero();
        if (opcao < 1 || opcao > N) continue;
        Item &it = tabela[buscarItem(produtos[opcao - 1])];
        if (it.tipo != "pocao" && it.quantidade > 0) cout << "Voce ja possui isso.\n";
        else if (ouro < it.preco) cout << "Ouro insuficiente!\n";
        else { ouro -= it.preco; it.quantidade++; cout << "Voce comprou " << it.nome << "!\n"; }
    }
}

void olhar() {
    cout << "\n=== " << nomeLocal[localAtual] << " ===\n";
    if (inimigo[localAtual].nome == "") return;
    if (!derrotado[localAtual]) cout << "Perigo! " << inimigo[localAtual].nome << " esta aqui!\n";
    else cout << "A regiao esta calma, mas criaturas ainda rondam (voce pode cacar).\n";
}

void mover() {
    cout << "\nPara onde deseja ir?\n";
    for (int i = 0; i < NUM_LOCAIS; i++) {
        if (!caminho[localAtual][i]) continue;
        cout << " " << i << ") " << nomeLocal[i];
        if (!caminhoLiberado(localAtual, i)) cout << " [BLOQUEADO - derrote " << inimigo[localAtual].nome << "]";
        cout << "\n";
    }
    cout << "> ";
    int destino = lerNumero();
    if (destino < 0 || destino >= NUM_LOCAIS || !caminho[localAtual][destino]) {
        cout << "Destino invalido.\n";
    } else if (!caminhoLiberado(localAtual, destino)) {
        cout << "Caminho bloqueado! Derrote " << inimigo[localAtual].nome << " primeiro.\n";
    } else {
        localAtual = destino;
        olhar();
        if (localAtual == 0) {
            vida = vidaMaxima;
            mana = manaMaxima;
            cout << "Voce descansa na estalagem e recupera vida e mana.\n";
        }
    }
}

// ============================================================
//  COMBATE (distancia, alcance de armas e magias)
// ============================================================
void recompensas(Inimigo &e) {
    int ganho = sorteio(e.ouroMin, e.ouroMax);
    ouro += ganho;
    cout << "\nVoce venceu " << e.nome << "! +" << ganho << " de ouro.\n";
    for (int d = 0; d < 2; d++) {
        if (e.drop[d] == "" || !chance(e.chance[d])) continue;
        int pos = buscarItem(e.drop[d]);
        if (pos == -1) continue;  // so existem itens ja cadastrados
        if (tabela[pos].tipo == "magia") {
            if (tabela[pos].quantidade > 0) continue;
            tabela[pos].quantidade = 1;
            cout << "Voce encontrou um grimorio e aprendeu: " << tabela[pos].nome << "!\n";
        } else {
            tabela[pos].quantidade++;
            cout << "Item obtido: " << tabela[pos].nome << "\n";
        }
    }
}

int escolherMagia() {  // posicao na tabela ou -1
    int posicoes[TAM], n = 0;
    cout << "\n--- Magias ---\n";
    for (int i = 0; i < TAM; i++)
        if (tabela[i].ocupado && tabela[i].tipo == "magia" && tabela[i].quantidade > 0) {
            posicoes[n++] = i;
            cout << " " << n << ") " << tabela[i].nome << " (dano " << tabela[i].ataque
                 << ", mana " << tabela[i].mana << ", alcance " << tabela[i].alcance << ")\n";
        }
    cout << " 0) Voltar\n> ";
    int op = lerNumero();
    return (op < 1 || op > n) ? -1 : posicoes[op - 1];
}

void combate() {
    Inimigo &e = inimigo[localAtual];
    if (e.nome == "") { cout << "Nao ha inimigos aqui.\n"; return; }
    bool cacada = derrotado[localAtual];  // luta repetida: so da recompensas
    int vidaE = e.vida, distancia = 5;
    cout << "\n*** Combate contra " << e.nome << " ***\n";
    if (cacada) cout << "(Uma nova criatura aparece.)\n";

    while (vida > 0 && vidaE > 0) {
        cout << "\nSua vida: " << vida << " | Mana: " << mana << " | Inimigo: " << vidaE
             << " | Distancia: " << distancia << " (seu alcance " << alcanceArma()
             << ", inimigo " << e.alcance << ")\n"
             << "1) Atacar  2) Magia  3) Pocao  4) Aproximar  5) Recuar  6) Fugir\n> ";
        int opcao = lerNumero();

        if (opcao == 6) { cout << "Voce fugiu!\n"; return; }
        else if (opcao == 1) {
            if (distancia > alcanceArma()) { cout << "Inimigo fora de alcance!\n"; continue; }
            int dano = ataqueTotal() + sorteio(0, 2);
            vidaE -= dano;
            cout << "Voce causou " << dano << " de dano.\n";
        } else if (opcao == 2) {
            int pos = escolherMagia();
            if (pos == -1) continue;
            if (mana < tabela[pos].mana) { cout << "Mana insuficiente!\n"; continue; }
            if (distancia > tabela[pos].alcance) { cout << "Fora do alcance da magia!\n"; continue; }
            mana -= tabela[pos].mana;
            int dano = tabela[pos].ataque + sorteio(0, 3);  // ignora armadura
            vidaE -= dano;
            cout << "Voce lancou " << tabela[pos].nome << " e causou " << dano << " de dano.\n";
        } else if (opcao == 3) {
            if (!usarPocao()) continue;
        } else if (opcao == 4) {
            if (distancia <= 1) { cout << "Voce ja esta colado no inimigo.\n"; continue; }
            cout << "Voce se aproxima. Distancia: " << --distancia << "\n";
        } else if (opcao == 5) {
            if (distancia >= 8) { cout << "Nao ha espaco para recuar.\n"; continue; }
            cout << "Voce recua. Distancia: " << ++distancia << "\n";
        } else continue;

        if (vidaE > 0) {  // turno do inimigo
            if (distancia <= e.alcance) {
                int dano = max(1, e.ataque + sorteio(-1, 1) - defesaTotal());
                vida -= dano;
                cout << e.nome << " causou " << dano << " de dano em voce.\n";
            } else {
                cout << e.nome << " avanca. Distancia: " << --distancia << "\n";
            }
        }
        mana = min(manaMaxima, mana + 2);
    }

    if (vida <= 0) { cout << "\nVoce foi derrotado... Fim de jogo.\n"; jogoAcabou = true; return; }
    recompensas(e);
    if (!cacada) derrotado[localAtual] = true;
    if (localAtual == 5) venceu = true;
}

// ============================================================
//  MONTAGEM DO MUNDO E LOOP PRINCIPAL
// ============================================================
void montarJogo() {
    //          nome                 tipo        atq def cura preco alc mana qtd
    inserirItem("Adaga Enferrujada", "arma",      1, 0, 0,  0,  1, 0, 1);
    inserirItem("Espada de Ferro",   "arma",      5, 0, 0,  30, 1, 0, 0);
    inserirItem("Espada de Aco",     "arma",      9, 0, 0,  70, 2, 0, 0);
    inserirItem("Arco Longo",        "arma",      6, 0, 0,  50, 5, 0, 0);
    inserirItem("Machado do Troll",  "arma",     13, 0, 0,  0,  2, 0, 0);
    inserirItem("Armadura de Couro", "armadura",  0, 2, 0,  25, 0, 0, 0);
    inserirItem("Cota de Malha",     "armadura",  0, 5, 0,  60, 0, 0, 0);
    inserirItem("Armadura Sombria",  "armadura",  0, 8, 0,  0,  0, 0, 0);
    inserirItem("Pocao de Cura",     "pocao",     0, 0, 30, 10, 0, 0, 2);
    inserirItem("Elixir Maior",      "pocao",     0, 0, 60, 25, 0, 0, 0);
    inserirItem("Faisca",            "magia",     8, 0, 0,  0,  4, 5, 1);   // ataque = dano
    inserirItem("Bola de Fogo",      "magia",    18, 0, 0,  80, 6, 12, 0);
    inserirItem("Raio Gelido",       "magia",    14, 0, 0,  0,  5, 9, 0);
    catalogoFechado = true;  // nenhum item novo pode ser criado daqui em diante

    for (int i = 0; i < NUM_LOCAIS; i++) derrotado[i] = (inimigo[i].nome == "");
    ligarLocais(0, 1); ligarLocais(1, 2); ligarLocais(1, 3);
    ligarLocais(2, 4); ligarLocais(3, 4); ligarLocais(4, 5);
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
        if (opcao == 1) olhar();
        else if (opcao == 2) mover();
        else if (opcao == 3) combate();
        else if (opcao == 4) mostrarInventario();
        else if (opcao == 5) equipar();
        else if (opcao == 6) usarPocao();
        else if (opcao == 7) loja();
        else if (opcao == 8) mostrarRota(localAtual, 5);
        else if (opcao == 0) break;
        else cout << "Opcao invalida.\n";
    }
    if (venceu) cout << "\n*** VITORIA! Eldoria esta salva! ***\n";
    return 0;
}
