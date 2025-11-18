#define CHEMIN_DOSSIER_DONNEES "D:/Document/code/c++/optimisationIaEmploiDuTemps/Data/"
#define NOM_FICHIER_LISTE_FICHIER_DONNEES "data.txt"
#define NOM_FICHIER_LISTE_SORTIE "sortie.txt"

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include "Instance.hpp"
#include "Solution.hpp"
#include "Algorithme.hpp"

#include <Windows.h>

using namespace std;

int Resolution(Instance * instance);

int main(int argc, const char* argv[]) {
    SetConsoleOutputCP(65001);
    try
    {
        string s_tmp;
        string s_chemin=CHEMIN_DOSSIER_DONNEES;
        s_chemin.append(NOM_FICHIER_LISTE_FICHIER_DONNEES);
        
        ifstream fichier(s_chemin.c_str(), std::ios::in);std::ofstream fichier_Sortie_Resume;
        s_chemin = CHEMIN_DOSSIER_DONNEES;
        s_chemin.append(NOM_FICHIER_LISTE_SORTIE);
        ofstream fichier_Sortie(s_chemin.c_str(), std::ios::out | std::ios::app);

        s_chemin = CHEMIN_DOSSIER_DONNEES;
        s_chemin.append("sortie.csv");
        ofstream fichier_Sortie_tab(s_chemin.c_str(), std::ios::out | std::ios::app);

        if(fichier)
        {
            if(fichier_Sortie && fichier_Sortie_tab)
            {
                fichier_Sortie_tab << endl;
                fichier_Sortie<<" Fichier données\t Tps de résolution \t Best solution"<<endl;
                getline(fichier,s_tmp);
                while(s_tmp!="")
                {
                    Instance * instance = new Instance();
                    chrono::time_point<chrono::system_clock> chrono_start, chrono_end;
                    chrono::duration<double> elapsed;
                    int i_best_solution_score=0;
                    s_chemin=CHEMIN_DOSSIER_DONNEES;
                    cout<< " Résolution de "<<s_tmp<<endl;
                    s_chemin.append(s_tmp);
                    s_chemin.erase(remove(s_chemin.begin(), s_chemin.end(), '\r'), s_chemin.end());
                    s_chemin.erase(remove(s_chemin.begin(), s_chemin.end(), '\n'), s_chemin.end());
                    
                    instance->chargement_Instance(s_chemin);
                    chrono_start = chrono::system_clock::now();
                    i_best_solution_score=Resolution(instance);
                    cout<< " Fin de résolution de "<<s_tmp<<endl;
                    chrono_end = chrono::system_clock::now();
                    elapsed=chrono_end-chrono_start;
                    fichier_Sortie << s_chemin << "\t" << elapsed.count() << "\t" << i_best_solution_score << endl;
                    fichier_Sortie_tab<< i_best_solution_score << ";";
                    s_tmp="";
                    getline(fichier,s_tmp);
                    delete instance;
                }
                fichier_Sortie.close();
                fichier_Sortie_tab.close();
            }
            else
            {
                cout<<" Erreur lecture des données : chemin vers la sortie non valide. "<<endl;
            }
            fichier.close();
        }
        else
        {
            cout<<" Erreur lecture des données : chemin listant l'ensemble des données non valide. "<<endl;
        }
    }
    
    catch(string err)
    {
        cout << "Erreur fatale : " <<endl;
        cout << err <<endl;
    }
    return 0;
}

int Resolution(Instance * instance)
{
    Algorithme algo = *new Algorithme(*instance);
    vector<vector<int>> solution = algo.algo_genetique();

    vector<int> resultat(1);
    algo.compter_erreur(resultat, solution);

    Solution* meilleurSolution = new Solution();
    for (int i = 0; i < solution.size(); i++) {
        meilleurSolution->v_v_IdShift_Par_Personne_et_Jour.push_back(solution[i]);
    }

    /*cout << "planning " << endl;
    for (int j = 0; j < solution.size(); j++) {
        cout << "personne " << j << " :\t";
        for (int k = 0; k < solution[0].size(); k++) {
            cout << solution[j][k] << "\t";
        }
        cout << endl;
    }*/

    meilleurSolution->i_valeur_fonction_objectif = resultat[0];
    meilleurSolution->Verification_Solution(instance);

    return meilleurSolution->i_valeur_fonction_objectif;    
}

