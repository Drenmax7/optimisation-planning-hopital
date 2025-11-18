#ifndef ALGORITHME
#define ALGORITHME 

#include "Instance.hpp"
#include "Solution.hpp"

#include <Windows.h>
#include <random>
#include <algorithm>
#include <iostream>
#include<vector>
#include <numeric>
#include <chrono>

#define TAILLE_POPULATION 10

#define tempsTravail 0
#define nbShiftSuite 1
#define jourOffSuite 2
#define premierJourTravail 3
#define premierJourOff 4
#define nbWE 5

using namespace std;

class Algorithme {
private:
	Instance& instance;

	mt19937 graine;

	int nbJour;
	int nbPersonne;
	int nbShift;

	std::chrono::time_point<std::chrono::high_resolution_clock> chronoStopGenerePersonne;


	// Index de mutation Initialise à 0 pour commencer par la première personne
	int indexPersonneMutation = 0;

	//donne pour un jour donné, pour un shift donné, la liste des personne travaillant ce jour là
	vector<vector<vector<int>>> emploi_du_temps;

	//donne pour chaque personne, pour chaque jour, si la personne est en conges
	vector<vector<bool>> jourConges;
	//donne pour chaque personne la liste de ses jour de congés
	vector<vector<int>> jourCongesListe;

	// shiftSuccInterdit[i][j] = 1 signifie que le shift j peut succeder au shift i
	vector<vector<bool>> shiftSuccInterdit;

	//donne la duree de chaque shift
	vector<int> dureeShift;
	//la duree du shift le plus long
	int plusLongShift;

	//la duree moyenne d'un shift
	int dureeMoyenneShift;

	//donne pour chaque personne la duree minimal et maximal de travail qu'il peut effectuer
	vector<vector<int>> dureeTravailPersonne;

	//donne pour chaque personne le nombre minimal et maximal de shift successif qu'il peut effectuer
	vector<vector<int>> nbShiftSuccessifPersonne;

	//donne pour chaque personne le nombre de jour off minimum qu'elle soit avoir
	vector<int> nbJourOffMinPersonne;

	//donne pour chaque personne le nombre maximal de we qu'elle peut travailler
	vector<int> nbMaxWeTravailPersonne;

	//donne pour chaque personne et pour chaque shift le nombre maximum de ce shift que la personne peut faire
	vector<vector<int>> nbShiftMax;

	//donne pour chaque personne, pour chaque jour, pour chaque shift, le poid a ajouter au score si la personne n'a pas ce shift
	vector<vector<vector<int>>> poidsVouloirShift;
	int sommePoidsVouloir;

	//donne pour chaque personne, pour chaque jour, pour chaque shift, le poid a ajouter au score si la personne a ce shift
	vector<vector<vector<int>>> poidsPasVouloirShift;

	//donne pour chaque jour, pour chaque shift, le nombre de personne optimal
	vector<vector<int>> nbPersonneRequisShift;

	//donne pour chaque jour, pour chaque shift, le poids infligé pour chaque personne en moins dans le shift
	vector<vector<int>> poidsPersonneManquante;

	//donne pour chaque jour, pour chaque shift, le poids infligé pour chaque personne en plus dans le shift
	vector<vector<int>> poidsPersonneSurplus;

	//variable pour la fonction generer personne n

	//garde en memoire l'historique des branches qui ont ete utilisé mais ont mené à un cul de sac
	vector<vector<int>> mauvaisesBranche;

	//note les shift qui sont disponible pour le jour actuel
	vector<bool> shiftDisponible;

	//garde en memoire l'etat des variable avant chaque exploration de branche
	vector<vector<int>> etatVariable;

public:
	// constructeur 
	Algorithme(Instance& instance);

	//initialise les listes
	void initialiser_listes();

	//implementation d'un algo genetique optimisant le score de planning d'hopital
	vector<vector<int>> algo_genetique();

	//genere une population initiale de façon aleatoire
	void genere_population_initiale(vector<vector<vector<int>>>& populationInitiale);

	// Genere un emploi du temps pour la ieme personne, sans erreur
	// nbPersonneParShift donne le nombre de personne dans chaque shift du planning dans lequel est la personne
	// La fonction part du principe que la personne n a ete exclue du compte du nombre de personne par shift
	// Si la personne n avait deja des shift attribué, ils doivent etre enlevé de nbPersonneParShift avant l'appel de fonction
	inline int generePersonneN(vector<int>& personne, int numPersonne, vector<vector<int>>& nbPersonneParShift);

	//prend en entrée une liste de planning et renvoie une liste de meme taille donnant le nombre d'erreur fatal et le score de chaque planning
	inline void compter_erreur(vector<int>& resultat, vector<vector<vector<int>>>& liste_planning);
	void compter_erreur(vector<int>& resultat, vector<vector<int>>& planning);

	//prend en entrée la population dans laquelle il faut choisir les parents ainsi qu'un vecteur representant leur performence
	inline void choisir_parent(vector<vector<vector<int>>>& parents, vector<vector<vector<int>>>& population, vector<int>& vect_nb_erreur);

	//prend en entree la liste des parents et va generer des enfants pour chaque couple de parent. (va donc generer listeParents.size()/2 * NOMBRE_ENFANT enfants)
	inline void genere_enfant(vector<vector<vector<int>>>& listeEnfants, vector<vector<vector<int>>>& listeParents);

	//operateur de croisement de la generation des enfants
	inline void operateurCroisement(vector<vector<int>>& enfant, vector<vector<int>>& p1, vector<vector<int>>& p2);

	// Applique une mutation sur les enfants proportionelle a la performence de leurs parents
	inline void appliquer_mutation(vector<vector<vector<int>>>& listeEnfants);
	inline void amelioreIndividu(vector<vector<int>>& individu);

	//deplace les enfants de la lsite d'enfant et les parents vers la population
	inline void updatePopulation(vector<vector<vector<int>>>& population, vector<vector<vector<int>>>& listeEnfant, vector<vector<vector<int>>>& listeParent);


	//une methode qui retourne l'indice le l'element qu'a le plus petit nb d'erreurs 
	inline int trouve_erreur_minimal(const vector<int>& vect_nb_erreur);
	inline int trouve_erreur_maximal(const vector<int>& vect_nb_erreur);
};




#endif // !ALGORITHME
