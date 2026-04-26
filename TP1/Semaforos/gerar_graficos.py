
import os
import glob
import csv
import statistics
import matplotlib.pyplot as plt

# Configuração visual global
plt.rcParams.update({
    "font.family": "DejaVu Sans",
    "font.size": 11,
    "axes.titlesize": 13,
    "axes.labelsize": 11,
    "legend.fontsize": 10,
    "figure.dpi": 150,
})

COMBINACOES  = [(1,1),(1,2),(1,4),(1,8),(2,1),(4,1),(8,1)]
TAMANHOS_N   = [1, 10, 100, 1000]
ROTULOS_COMBO = [f"({np},{nc})" for np, nc in COMBINACOES]

# Gráfico de tempo médio de execução
def gerar_grafico_tempo(caminho_csv="resultados_tempo.csv"):
    if not os.path.exists(caminho_csv):
        print(f"[AVISO] '{caminho_csv}' não encontrado. Pulando gráfico de tempo.")
        return

    dados = {}
    with open(caminho_csv, newline="") as f:
        for linha in csv.DictReader(f):
            chave = (int(linha["N"]), int(linha["Np"]), int(linha["Nc"]))
            dados.setdefault(chave, []).append(float(linha["tempo_seg"]))

    fig, ax = plt.subplots(figsize=(11, 6))
    cores     = plt.cm.tab10.colors
    marcadores = ["o", "s", "^", "D"]

    for idx_n, N in enumerate(TAMANHOS_N):
        medias, erros = [], []
        for np, nc in COMBINACOES:
            tempos = dados.get((N, np, nc), [])
            medias.append(statistics.mean(tempos) if tempos else None)
            erros.append(statistics.stdev(tempos) if len(tempos) > 1 else 0)

        ax.errorbar(
            range(len(COMBINACOES)), medias, yerr=erros,
            label=f"N = {N}",
            color=cores[idx_n], marker=marcadores[idx_n],
            linewidth=2, markersize=7, capsize=4,
        )

    ax.set_title("Tempo médio de execução × Combinação de threads (Np, Nc)")
    ax.set_xlabel("Combinação (Np, Nc)")
    ax.set_ylabel("Tempo médio (s)")
    ax.set_xticks(range(len(COMBINACOES)))
    ax.set_xticklabels(ROTULOS_COMBO)
    ax.legend(title="Tamanho do buffer N")
    ax.grid(axis="y", linestyle="--", alpha=0.5)
    plt.tight_layout()
    plt.savefig("grafico_tempo_medio.png")
    plt.close()
    print("[OK] grafico_tempo_medio.png")

# Gráficos de ocupação do buffer
def gerar_graficos_ocupacao():
    arquivos = sorted(glob.glob("ocupacao_N*_Np*_Nc*.csv"))
    if not arquivos:
        print("[AVISO] Nenhum arquivo de ocupação encontrado.")
        return

    for caminho in arquivos:
        nome   = os.path.basename(caminho).replace(".csv", "")
        partes = nome.split("_")        # 'ocupacao','N10','Np2','Nc4'
        N  = int(partes[1][1:])
        Np = int(partes[2][2:])
        Nc = int(partes[3][2:])

        ops, ocups = [], []
        with open(caminho, newline="") as f:
            for linha in csv.DictReader(f):
                ops.append(int(linha["operacao"]))
                ocups.append(int(linha["ocupacao"]))

        if not ops:
            continue

        # amostra até 5000 pontos para não sobrecarregar o plot
        MAX_PONTOS = 5000
        if len(ops) > MAX_PONTOS:
            passo = len(ops) // MAX_PONTOS
            ops   = ops[::passo]
            ocups = ocups[::passo]

        media_ocup = statistics.mean(ocups)

        fig, ax = plt.subplots(figsize=(11, 4))
        ax.fill_between(ops, ocups, alpha=0.25, color="steelblue")
        ax.plot(ops, ocups, linewidth=0.8, color="steelblue")
        ax.axhline(N, color="red", linestyle="--", linewidth=1.2,
                   label=f"Capacidade máxima (N={N})")
        ax.axhline(media_ocup, color="orange", linestyle=":", linewidth=1.5,
                   label=f"Ocupação média ({media_ocup:.1f})")

        ax.set_title(f"Ocupação do buffer ao longo do tempo — N={N}, Np={Np}, Nc={Nc}")
        ax.set_xlabel("Operação (produção ou consumo)")
        ax.set_ylabel("Itens no buffer")
        ax.set_ylim(bottom=0, top=N + 1)
        ax.legend(loc="upper right")
        ax.grid(axis="y", linestyle="--", alpha=0.4)
        plt.tight_layout()

        saida = f"grafico_{nome}.png"
        plt.savefig(saida)
        plt.close()
        print(f"[OK] {saida}")


if __name__ == "__main__":
    print("=== Gerando gráficos do TP1 Parte 3 ===\n")
    gerar_grafico_tempo()
    print()
    gerar_graficos_ocupacao()
    print("\n=== Concluído! ===")
