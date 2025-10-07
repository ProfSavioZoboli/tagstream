#include <arduino.h>
#include "emprestimos.h"
#include "config.h"
#include <string>

typedef struct Equipamento_
{
    int numero;
    enum SituacaoEquipamento
    {
        OCUPADO,
        LIVRE,
        MANUTENCAO
    } situacao;
} Equipamento;

Equipamento inventario[MAX_EQUIPAMENTOS];

bool verificaDisponibilidade(int num_inicial, int num_final)
{
    if (num_inicial > 0 && num_inicial < MAX_EQUIPAMENTOS && num_final > 0 && num_final < MAX_EQUIPAMENTOS)
    {
        if (num_final > num_inicial)
        {
            for (int i = num_inicial; i <= num_final; i++)
            {
                if (inventario[i].situacao != LIVRE)
                {
                    return false;
                }
            }
            return true;
        }
        else if (num_final == num_inicial)
        {
            return inventario[num_inicial].situacao == LIVRE;
        }
        else
        {
            return false;
        }
    }else{
        return false;
    }
}

void pegaEmprestado(int equipamento){
    inventario[equipamento].situacao = OCUPADO;
}

bool handleEmprestimo(char* query){
    int primeiro;
    int ultimo;
    std::string str_query = query;
    if(str_query.find('A')!= -1){
        int index_a = str_query.find('A');
        primeiro = std::stoi(str_query.substr(0,index_a));
        ultimo = std::stoi(str_query.substr(index_a,str_query.size()));
    }else{
        primeiro = std::stoi(str_query);
        ultimo = primeiro;
    }
    if(verificaDisponibilidade(primeiro,ultimo)){
        for(int i=primeiro;i<=ultimo;i++){
            pegaEmprestado(i);
        }
    }
}
