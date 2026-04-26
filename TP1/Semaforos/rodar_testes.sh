BINARIO="./sem_pc"
FONTE="sem_produtor_consumidor.c"
CSV_TEMPO="resultados_tempo.csv"
REPETICOES=10  # número de execuções por combinação

# Combinações de (Np, Nc) exigidas pelo enunciado
COMBINACOES=(
    "1 1"
    "1 2"
    "1 4"
    "1 8"
    "2 1"
    "4 1"
    "8 1"
)

# Valores de N (tamanho do buffer)
TAMANHOS_N=(1 10 100 1000)

# verifica/compila o binário
if [ ! -f "$BINARIO" ]; then
    echo "setup binário não encontrado, compilando"
    gcc -O2 -o sem_pc "$FONTE" -lpthread -lm
    if [ $? -ne 0 ]; then
        echo "ERRO, falha na compilação"
        exit 1
    fi
    echo "setup compilação concluída"
fi

# cria cabeçalho do CSV de tempos
echo "N,Np,Nc,repeticao,tempo_seg" > "$CSV_TEMPO"

# contadores para progresso
total_runs=$(( ${#TAMANHOS_N[@]} * ${#COMBINACOES[@]} * REPETICOES ))
run_atual=0

echo " Benchmark Produtor-Consumidor com Semáforos"
echo " Total de execuções: $total_runs"

for N in "${TAMANHOS_N[@]}"; do
    for combo in "${COMBINACOES[@]}"; do
        NP=$(echo $combo | awk '{print $1}')
        NC=$(echo $combo | awk '{print $2}')

        echo ""
        echo ">>> N=$N | Np=$NP | Nc=$NC"

        for rep in $(seq 1 $REPETICOES); do
            run_atual=$((run_atual + 1))
            echo -n "    execução $rep/$REPETICOES ... "

            saida=$($BINARIO $N $NP $NC --silencioso 2>&1)
            tempo=$(echo "$saida" | grep "^TEMPO_SEG=" | cut -d'=' -f2)

            if [ -z "$tempo" ]; then
                echo "FALHOU (tempo não capturado)"
                continue
            fi

            echo "${tempo}s"
            echo "$N,$NP,$NC,$rep,$tempo" >> "$CSV_TEMPO"
        done
    done
done

echo ""
echo " Benchmark concluído"
echo " → Tempos salvos em: $CSV_TEMPO"
echo " → CSVs de ocupação: ocupacao_N*_Np*_Nc*.csv"

