# Trabalho Prático 1

**Sistemas Distribuídos — CEFET-MG**  
**Alunos:** Pedro Henrique Ferreira Santos e Thiago Leonardo Oliveira Bertolino

---

## Estrutura do Repositório

```
├── pipe_produtor_consumidor.c     # Parte 1: Produtor-Consumidor com Pipes
├── sem_produtor_consumidor.c      # Parte 2: Produtor-Consumidor com Semáforos
├── rodar_testes.sh              # Script de benchmark automático (Parte 2)
└── gerar_graficos.py            # Script de geração de gráficos (Parte 2)
```

---

## Parte 1 — Pipes

### Compilar
```bash
gcc -o pipe_pc pipe_produtor_consumidor.c
```

### Usar
```bash
./pipe_pc <quantidade_de_numeros>
```

### Exemplos
```bash
./pipe_pc 10      # 10 números
./pipe_pc 100     # 100 números
./pipe_pc 1000    # 1000 números
```

---

## Parte 2 — Produtor-Consumidor com Semáforos

### Dependências
```bash
sudo apt install gcc python3-matplotlib -y
```

### Compilar
```bash
gcc -O2 -o sem_pc sem_produtor_consumidor.c -lpthread -lm
```

### Usar
```bash
./sem_pc <N> <Np> <Nc> [M] [--silencioso]
```

| Parâmetro | Descrição |
|---|---|
| `N` | Tamanho do buffer compartilhado |
| `Np` | Número de threads produtoras |
| `Nc` | Número de threads consumidoras |
| `M` | Quantidade de números a processar (padrão: 100000) |
| `--silencioso` | Suprime saída por item (usado no benchmark) |

### Exemplos
```bash
# Teste rápido com 20 números
./sem_pc 10 1 1 20

# Estudo de caso completo (100 mil números)
./sem_pc 10 2 4

# Benchmark silencioso
./sem_pc 10 2 4 100000 --silencioso
```

### Rodar o benchmark completo (280 execuções)
```bash
chmod +x rodar_testes.sh
./rodar_testes.sh
```
> Tempo estimado: 20 a 30 minutos. Gera `resultados_tempo.csv` e arquivos `ocupacao_N*_Np*_Nc*.csv`.

### Gerar os gráficos
```bash
python3 gerar_graficos.py
```
> Gera `grafico_tempo_medio.png` e `grafico_ocupacao_*.png` na pasta atual.
