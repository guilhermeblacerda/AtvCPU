# AtvCPU

## Arquivos do Projeto
- `escalonador.c` - arquivo com a lógica
- `escalonador.h` - arquivo header com structs e protótipos
- `Makefile` - Instruções de compilação
- `README.md` - Este arquivo de documentação
- 'evidencias.log' - terminal

## Funcionalidades Implementadas
- Execução rate-monotonic
- Execução earliest-deadline-first
- Tratamento de erros

Compilação
```bash
make
```


Execução
```bash
././scheduler rate voo.txt

OU
```bash
./scheduler edf voo.txt
```
