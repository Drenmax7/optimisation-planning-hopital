#include "Algorithme.hpp"




//constructeur de la classe
Algorithme::Algorithme(Instance& instance) : instance(instance) {
    srand((unsigned int)time(NULL));

    //initialiser la matrice de l'emplo de temps 
    int nb_jours = instance.get_Nombre_Jour();
    int nb_shifts = instance.get_Nombre_Shift();
    emploi_du_temps = vector<vector<vector<int>>>(nb_jours, vector<vector<int>>(nb_shifts, vector<int>()));

    initialiser_listes();

    //vector pour la fonction generer personne n
    etatVariable = vector<vector<int>>(nbJour, vector<int>(6, 0));
    mauvaisesBranche = vector<vector<int>>(nbJour, vector<int>());
    shiftDisponible = vector<bool>(nbShift + 1);
}

void Algorithme::initialiser_listes() {
    nbPersonne = instance.get_Nombre_Personne();
    nbJour = instance.get_Nombre_Jour();
    nbShift = instance.get_Nombre_Shift();

    jourConges = vector<vector<bool>>(nbPersonne, vector<bool>(nbJour, 0));
    jourCongesListe = vector<vector<int>>(nbPersonne);

    for (int i = 0; i < nbPersonne; i++) {
        vector<int> congesParPersonne = instance.get_vector_Personne_Id_Jour_Conges(i);
        jourCongesListe[i] = congesParPersonne;
        for (int j = 0; j < congesParPersonne.size(); j++) {
            int jour = congesParPersonne[j];
            jourConges[i][jour] = 1;
        }
    }


    shiftSuccInterdit = vector<vector<bool>>(nbShift, vector<bool>(nbShift, 1));

    for (int i = 0; i < nbShift; i++) {
        vector<int> interdit = instance.get_vector_Shift_Suc_Interdit(i);
        for (int j = 0; j < interdit.size(); j++) {
            int shift = interdit[j];
            shiftSuccInterdit[i][shift] = 0;
        }
    }

    dureeShift = vector<int>(nbShift);
    dureeMoyenneShift = 0;

    for (int i = 0; i < nbShift; i++) {
        int duree = instance.get_Shift_Duree(i);
        dureeShift[i] = duree;
        dureeMoyenneShift += duree;
    }
    dureeMoyenneShift /= nbShift;
    plusLongShift = *max_element(dureeShift.begin(), dureeShift.end());


    dureeTravailPersonne = vector<vector<int>>(nbPersonne, vector<int>(2));

    for (int i = 0; i < nbPersonne; i++) {
        int minimal = instance.get_Personne_Duree_total_Min(i);
        int maximal = instance.get_Personne_Duree_total_Max(i);
        dureeTravailPersonne[i][0] = minimal;
        dureeTravailPersonne[i][1] = maximal;
    }

    nbShiftSuccessifPersonne = *new vector<vector<int>>(nbPersonne, vector<int>(2));

    for (int i = 0; i < nbPersonne; i++) {
        int minimal = instance.get_Personne_Nbre_Shift_Consecutif_Min(i);
        int maximal = instance.get_Personne_Nbre_Shift_Consecutif_Max(i);
        nbShiftSuccessifPersonne[i][0] = minimal;
        nbShiftSuccessifPersonne[i][1] = maximal;
    }

    nbJourOffMinPersonne = *new vector<int>(nbPersonne);

    for (int i = 0; i < nbPersonne; i++) {
        int minimal = instance.get_Personne_Jour_OFF_Consecutif_Min(i);
        nbJourOffMinPersonne[i] = minimal;
    }

    nbMaxWeTravailPersonne = *new vector<int>(nbPersonne);

    for (int i = 0; i < nbPersonne; i++) {
        int maximal = instance.get_Personne_Nbre_WE_Max(i);
        nbMaxWeTravailPersonne[i] = maximal;
    }

    nbShiftMax = *new vector<vector<int>>(nbPersonne, vector<int>(nbShift));

    for (int i = 0; i < nbPersonne; i++) {
        for (int j = 0; j < nbShift; j++) {
            int maximal = instance.get_Personne_Shift_Nbre_Max(i, j);
            nbShiftMax[i][j] = maximal;
        }
    }

    poidsPasVouloirShift = *new vector<vector<vector<int>>>(nbPersonne, vector<vector<int>>(nbJour, vector<int>(nbShift, 0)));
    poidsVouloirShift = *new vector<vector<vector<int>>>(nbPersonne, vector<vector<int>>(nbJour, vector<int>(nbShift, 0)));
    sommePoidsVouloir = 0;

    for (int i = 0; i < nbPersonne; i++) {
        for (int j = 0; j < nbJour; j++) {
            for (int k = 0; k < nbShift; k++) {
                int poidPasVouloir = instance.get_Poids_Refus_Pers_Jour_Shift(i, j, k);
                int poidVouloir = instance.get_Poids_Affectation_Pers_Jour_Shift(i, j, k);

                poidsPasVouloirShift[i][j][k] = poidPasVouloir;
                poidsVouloirShift[i][j][k] = poidVouloir;

                sommePoidsVouloir += poidVouloir;
            }
        }
    }

    nbPersonneRequisShift = *new vector<vector<int>>(nbJour, vector<int>(nbShift));
    poidsPersonneManquante = *new vector<vector<int>>(nbJour, vector<int>(nbShift));
    poidsPersonneSurplus = *new vector<vector<int>>(nbJour, vector<int>(nbShift));

    for (int i = 0; i < nbJour; i++) {
        for (int j = 0; j < nbShift; j++) {
            int requis = instance.get_Nbre_Personne_Requis_Jour_Shift(i, j);
            int poidManquant = instance.get_Poids_Personne_En_Moins_Jour_Shift(i, j);
            int poidSurplus = instance.get_Poids_Personne_En_Plus_Jour_Shift(i, j);

            nbPersonneRequisShift[i][j] = requis;
            poidsPersonneManquante[i][j] = poidManquant;
            poidsPersonneSurplus[i][j] = poidSurplus;
        }
    }
}

vector<vector<int>> Algorithme::algo_genetique() {


    // Conditions d'arrêt et initialisation
    int nombreIteration = 100;  // Nombre maximum d'itérations
    int indice = 0;                 // Index de la meilleure solution
    int generation = 0;         // Compteur de générations

    // Démarrer un chronomètre pour limiter le temps d'exécution
    auto start = chrono::high_resolution_clock::now();

    // Générer la première population de solutions
    vector<vector<vector<int>>> population(
        TAILLE_POPULATION, vector<vector<int>>(nbPersonne, vector<int>(nbJour,-1))
    );
    genere_population_initiale(population);
    

    // Calculer les erreurs pour chaque solution de la population
    vector<int> vect_nb_erreur(TAILLE_POPULATION);
    compter_erreur(vect_nb_erreur, population);


    // Initialiser les parents et les enfants
    vector<vector<vector<int>>> parents(
        TAILLE_POPULATION / 2, vector<vector<int>>(nbPersonne, vector<int>(nbJour))
    );
    vector<vector<vector<int>>> listeEnfant(
        TAILLE_POPULATION / 2, vector<vector<int>>(nbPersonne, vector<int>(nbJour))
    );

    //contient la meilleur solution obtenu et son score
    //on met une valeur de base dans le cas tres peu probable ou on est sur l'instance 16 ( la seul ou j'ai eu le probleme) et que des la premiere generation
    //la fonction de mutation ne passe pas et bloque entrainant le bricolage d'echanger l'enfant defectueux avec la meilleur solution
    vector<vector<int>> meilleurSolution = population[0];
    unsigned int meilleurScore = -1;

    while (generation < nombreIteration) {

        auto now = chrono::high_resolution_clock::now();
        // Vérifier le temps écoulé
        auto elapsedTime = chrono::duration_cast<chrono::seconds>(now - start).count();

        // Si 60 secondes se sont écoulées, arrêter l'algorithme
        if (elapsedTime >= 59) {
            std::cout << "Temps limite atteint : " << elapsedTime << " secondes." << std::endl;
            break;
        }

        // Sélection des meilleurs parents (roulette)
        choisir_parent(parents, population, vect_nb_erreur);

        // Générer des enfants par croisement
        genere_enfant(listeEnfant, parents);

        // Appliquer une mutation sur les enfants proportionnelle à la performance des parents
        appliquer_mutation(listeEnfant);

        // Mettre à jour la population avec les nouveaux enfants
        updatePopulation(population, listeEnfant, parents);

        // Calculer les erreurs pour chaque solution de la population
        compter_erreur(vect_nb_erreur, population);

        // Trouver l'indice de la solution avec le score minimum (meilleure solution)
        indice = trouve_erreur_minimal(vect_nb_erreur);

        if (vect_nb_erreur[indice] < meilleurScore) {
            meilleurScore = vect_nb_erreur[indice];
            meilleurSolution = population[indice];
            std::cout << "\tscore : " << vect_nb_erreur[indice] << std::endl;
        }

        if (generation % 100 == 0) {
            std::cout << "generation : " << generation << std::endl;
        }

        generation++;
    }

    // Retourner la meilleure solution trouvée dans la population
    return meilleurSolution;
}

void Algorithme::genere_population_initiale(vector<vector<vector<int>>>& populationInitiale) {
    for (int i = 0; i < TAILLE_POPULATION; i++) {
        //if (i != 0) continue;
        //cout << "k  " << 0 << ", pop " << i << endl;
        // 
        //donne pour chaque individu de la population le nombre de personne dans chaque shift
        vector<vector<int>> nbPersonneParShift(nbJour, vector<int>(nbShift, 0));
        for (int j = 0; j < nbPersonne; j++) {
            //if (nbPersonne != 45) continue;
            //cout << "individu " << i << ", personne " << j << endl;
            generePersonneN(populationInitiale[i][j], j, nbPersonneParShift);

            //ajoute les jours qu'a fait la personne au nombre de personne par shift
            for (int k = 0; k < nbJour; k++) {
                int shiftJour = populationInitiale[i][j][k];
                if (shiftJour != -1) {
                    nbPersonneParShift[k][shiftJour]++;
                }
            }
        }
        amelioreIndividu(populationInitiale[i]);
    }
}


/*
genere la planning d'une personne
meme fonctionnement qu'une fonction recursive mais sans appel de fonction
fais la liste des shifts pouvant etre placé au jour 1 et en met un au hasard, puis procede au jour suivant de la meme maniere
si aucun shift ne peut etre placé, place un jour de congé
si auucun jour de congé ne peut etre placé revient au choix du jour precedant et empeche le choix qui avait ete fait

les shift peuvent etre invalidé a cause de toutes les contraintes fortes du problemes
seul la contrainte de la duree minimum de travail pose probleme

cette condition ne peut etre verifié qu'a la fin, et parfois c'est un changement au debut de l'emploi du temps qu'il faut faire

optimisation effectués pour palier a ce probleme :
    -calcul du nombre de jour de travail pouvant encore etre placé dans le meilleur des cas (en tenant compte des jours off qui seront forcement mis)
et force le retour en arriere si il ne reste pas assez de temps de travail possible pour depasser le minimum
    -pose un nombre maximum de retour en arriere que la fonction peut faire, puis reset la solution a 0 lorsque ce maximum est atteint
    -commencer la generation depuis un jour au hasard. On force les deux premiers jours a etre des jours de congés pour faciliter la lisaison entre le debut
et la fin
    - il y a de nombreuses boucles pour render indisponible tous les shifts, l'optimisation est de faire en sorte qu'une seule boucle s'execute

optimisation tenté mais echoués :
    -revenir de plusieurs iterations au lieu de 1. Probleme rencontré : invalide trop rapidement les chemins et fini bloquer et ne trouve pas de solutions
    -eviter les we avec un seul jour de travail. Probleme : difficile de gerer la combinaisons des congés avec le nombre de jour min et max succecif
    -renvoyer le nombre d'iteration que la fonction a fait avant de trouver une solution et renvoyer ce nombre. Le but est que le nombre soit recupéré et fourni
a la fonction lors de la prochaine generation de la meme personne, permettant d'eviter les etapes de croissance progressive du max d'iterations. Probleme : entre
en conflit avec l'optimisation de commencer a un jour aleatoire qui repose sur le principe de recommencer de nombreuses fois de 0
    - augmente le maximum d'iterations de la fonction a chaque reset de solution. Probleme : entre en conflit avec le principe de depart aleatoire
*/

/*
optimisation de score:
    -compter le nombre de personne assigné aux shift de chaque jour. Lors de la verification sur l'eligibilité des shifts, l'algo verifie pour
chaque shift si le nombre de personne requis pour le shift est atteint. Si c'est le cas il rend le shift indisponible
Pour eviter d'invalider enormement de branches, l'algo remet disponible le shift depassant le moins le quota si aucun autre shift n'est possible.
    -Ne rend pas systematiquement disponible le shift depassant le moins le quota : n'ameliore pas beaucoup le score a part pour l'instance 1 et 18
*/

/*
choisir un jour au hasard
placer un jour de conge ou de travail
finir la serie
si la serie n'a pas depassé le minimum, verifier que assez de jour off/ttravail peuvent etre mis avant
si la serie a atteint le maximum, verifier qu'un jour off peut etre mis avant
avoir une variable qui donne la longeur de la premiere serie
verfier les we, possible probleme si l'algo commence un dimanche, 2 check de we : check nb max we, au moment de l'attribution du travail sur le planning

*/
int Algorithme::generePersonneN(vector<int>& personne, int numPersonne, vector<vector<int>>& nbPersonneParShift) {
    //pour s'assurer que la fonction ne dure pas trop longtemps
    chronoStopGenerePersonne = chrono::high_resolution_clock::now();

    //exploration d'arbre en profondeur randomisé à départ variable
    bool solutionValide = false;
    //combien de fois l'algo est autorisé a revenir en arriere
    int maxExploration = 100;
    while (!solutionValide) {
        //verifie que la fonction n'a pas duré plus de 1s ( c'est enorme)
        auto elapsedTime = chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - chronoStopGenerePersonne).count();
        if (elapsedTime > 1) {
            return 1;
        }

        solutionValide = true;

        vector<int> shiftRestant = nbShiftMax[numPersonne];

        //choisis au hasard un jour de debut de generation
        int debut = rand() % nbJour;

        mauvaisesBranche[debut].clear();

        //remet a 0 les variables qui ont ete modifié par la fonction lors d'autres appels
        etatVariable[debut][tempsTravail] = 0;
        etatVariable[debut][nbShiftSuite] = 0;
        etatVariable[debut][jourOffSuite] = 0;
        etatVariable[debut][premierJourTravail] = 0;
        etatVariable[debut][premierJourOff] = 0;
        etatVariable[debut][nbWE] = 0;


        int nbExploration = 0;
        for (int i = 0; i < nbJour; i++) {
            int numJour = (debut + i) % nbJour;
            int numVeille = (debut + i - 1 + nbJour) % nbJour;
            int numAvantVeille = (debut + i - 2 + nbJour) % nbJour;
            int numLendemain = (debut + i + 1) % nbJour;
            bool tousShift0 = false;


            if (numJour == 0) {
                etatVariable[numJour][nbShiftSuite] = 0;
                etatVariable[numJour][jourOffSuite] = 0;
                etatVariable[numJour][premierJourTravail] = 0;
                etatVariable[numJour][premierJourOff] = 0;
            }

            //reset la liste des shifts disponible
            for (int j = 0; j <= nbShift; j++) {
                shiftDisponible[j] = true;
            }

            //on empeche le choix de chaque shift ayant mené a un cul de sac
            for (int j = 0; j < mauvaisesBranche[numJour].size(); j++) {
                int mauvaisShift = mauvaisesBranche[numJour][j];


                if (mauvaisShift == -1) {
                    shiftDisponible[nbShift] = false;
                }
                else {
                    shiftDisponible[mauvaisShift] = false;
                }
            }

            //on force les 2 premiers jours a etre des congés
            if (!tousShift0 && i < nbJourOffMinPersonne[numPersonne]) {
                for (int j = 0; j < nbShift; j++) {
                    shiftDisponible[j] = 0;
                }
                tousShift0 = true;
            }


            //coche comme indisponible des shift selon les criteres
            //shift successif impossible
            //trop d'heure de travail
            //nb we max
            //nb shift max a la suite
            //nb shift min a la suite
            //nb jour off min
            //nb max d'un shift
            //jour off

            //empeche de prendre un shift interdit par le precedant shift
            //numVeille == numJour-1 pour eviter de considerer le dernier jour du planning alors qu'on vient de revenir au debut
            //numJour != debut pour ne pas considerer un jour qui n'a pas été mis
            if (numVeille == numJour - 1 && numJour != debut) { //i > 0
                int shiftVeille = personne[numVeille];
                if (shiftVeille != -1) {
                    //cout << "shift interdit" << endl;
                    for (int j = 0; j < nbShift; j++) {
                        if (!shiftSuccInterdit[shiftVeille][j]) {
                            shiftDisponible[j] = 0;
                        }
                    }
                }
            }

            //empeche le choix de shift qui ferait depasser la duree maximale de travail
            for (int j = 0; j < nbShift; j++) {
                if (etatVariable[numJour][tempsTravail] + dureeShift[j] > dureeTravailPersonne[numPersonne][1]) {
                    //cout << "duree max" << endl;
                    shiftDisponible[j] = 0;
                }
            }


            //empeche le choix de jour de travail de we si ca ferait depasser le nombre de we max
            if (!tousShift0 && ((numJour % 7 == 5 || (numJour % 7 == 6 && personne[numVeille] == -1 && numJour != debut)) && etatVariable[numJour][nbWE] == nbMaxWeTravailPersonne[numPersonne])) {
                //cout << "we max" << endl;
                for (int j = 0; j < nbShift; j++) {
                    shiftDisponible[j] = 0;
                }
                tousShift0 = true;
            }

            //empeche le choix d'un jour de travail si le maximum de shift successif est depassé
            if (!tousShift0 && etatVariable[numJour][nbShiftSuite] == nbShiftSuccessifPersonne[numPersonne][1]) {
                //cout << "shift sucessif max" << endl;
                for (int j = 0; j < nbShift; j++) {
                    shiftDisponible[j] = 0;
                }
                tousShift0 = true;
            }

            //si la personne n'a pas fait assez de shift a la suite, empeche le jour de congé
            //check si premierJourOff = true pour eviter le cas des jour de travail au debut qui ne comptent pas
            //check si jourOffSuite = 0 pour eviter le cas ou 2 jours off sont consecutif
            if (etatVariable[numJour][nbShiftSuite] < nbShiftSuccessifPersonne[numPersonne][0] && etatVariable[numJour][premierJourOff] && etatVariable[numJour][jourOffSuite] == 0) {
                //cout << "shift sucessif min" << endl;
                shiftDisponible[nbShift] = 0;
            }

            //permet de faire la liaison entre la fin et le debut
            //comme le jour de debut est un jour off, il faut qu'au moment de l'atteindre il y ait assez de jour travaillé a la suite
            if (!tousShift0 && numJour < debut && debut - numJour + etatVariable[numJour][nbShiftSuite] < nbShiftSuccessifPersonne[numPersonne][0]) {
                //cout << "liaison debut fin" << endl;
                for (int j = 0; j < nbShift; j++) {
                    shiftDisponible[j] = 0;
                }
                tousShift0 = true;
            }

            //empeche le choix d'un jour de travail si le nombre de  jour off min n'est pas respecté
            if (!tousShift0 && etatVariable[numJour][jourOffSuite] < nbJourOffMinPersonne[numPersonne] && etatVariable[numJour][premierJourTravail] && etatVariable[numJour][nbShiftSuite] == 0) {
                //cout << "jour off min" << endl;
                for (int j = 0; j < nbShift; j++) {
                    shiftDisponible[j] = 0;
                }
                tousShift0 = true;
            }

            //empeche le choix d'un shift si la personne en a deja fait le maximum
            for (int j = 0; j < nbShift; j++) {
                if (shiftRestant[j] == 0) {
                    //cout << "qte shift max" << endl;
                    shiftDisponible[j] = 0;
                }
            }


            //empeche le choix d'un jour de travail si la personne doit avoir un congé
            if (!tousShift0 && jourConges[numPersonne][numJour]) {
                //cout << "conge" << endl;
                for (int j = 0; j < nbShift; j++) {
                    shiftDisponible[j] = 0;
                }
                tousShift0 = true;
            }

            //cree un vector contenant tous les shift possible
            vector<int> listeShiftDisponible;
            unsigned int personneTropMin = -1;
            int shiftMoinsFourni = -1;
            for (int j = 0; j < nbShift; j++) {
                if (shiftDisponible[j]) {
                    //empeche le choix d'un shift deja plein
                    if (nbPersonneRequisShift[numJour][j] > nbPersonneParShift[numJour][j]) {
                        listeShiftDisponible.push_back(j);
                    }
                    else {
                        //retient le shift plein le moins plein au cas ou aucun autre shift ne soit possible
                        int personneTrop = nbPersonneParShift[numJour][j] - nbPersonneRequisShift[numJour][j];
                        if (personneTrop < personneTropMin) {
                            personneTropMin = personneTrop;
                            shiftMoinsFourni = j;
                        }
                    }
                }
            }
            //rend disponible le shift plein le moins plein si aucun autre ne l'est
            if (listeShiftDisponible.size() == 0 && shiftMoinsFourni != -1 && rand() % 10 < 9) { // 9/10 chance
                listeShiftDisponible.push_back(shiftMoinsFourni);
            }

            //listeShiftDisponible.size() == 0 pour ne mettre un jour de congé que si il n'y a pas de shift possible
            if (listeShiftDisponible.size() == 0 && shiftDisponible[nbShift]) {
                listeShiftDisponible.push_back(-1);

            }     




            //si aucun shift n'est possible alors on revient
            bool revenir = false;
            if (listeShiftDisponible.size() == 0) {
                revenir = true;
            }
            else {
                //choisi aleatoirement un shift parmi ceux possible
                int choixShift = listeShiftDisponible[rand() % listeShiftDisponible.size()];
                //cout << "choix " << choixShift << ", num jour " << numJour << endl;

                //place le shift dans le planning et met a jour les variables
                if (choixShift == -1) {
                    personne[numJour] = -1;

                    etatVariable[numJour][jourOffSuite]++;
                    etatVariable[numJour][nbShiftSuite] = 0;
                    etatVariable[numJour][premierJourOff] = 1;
                }
                else {
                    personne[numJour] = choixShift;
                    shiftRestant[choixShift]--;

                    etatVariable[numJour][premierJourTravail] = 1;
                    etatVariable[numJour][tempsTravail] += dureeShift[choixShift];
                    etatVariable[numJour][jourOffSuite] = 0;
                    etatVariable[numJour][nbShiftSuite]++;

                    //si il s'agit d'un samedi ou alors d'un dimanche avec repos la veille
                    if (numJour % 7 == 5 || (numJour % 7 == 6 && personne[numVeille] == -1 && numJour != debut)) {
                        etatVariable[numJour][nbWE]++;
                    }
                }

                /*if (numPersonne == 21 || 1) {
                    cout << "planning " << numPersonne << " : ";
                    for (int k = 0; k < nbJour; k++) {
                        cout << personne[k] << ", ";
                    }
                    cout << endl;
                }*/

                //verifie si il reste assez de jour pour que la personne puisse atteindre son quota           

                //nombre de jour pouvant encore etre travaillé avant d'arriver a la fin du planning ou que la personne doivent etre mis en congé

                int jourRestant;
                int jourManquant;
                int tempsTravailRestantPossible = 0;
                if (numJour >= debut) {
                    //jour apres le debut
                    int jourRestantApres = nbJour - numJour - 1;
                    //cout << "jour restant apres " << jourRestantApres << endl;

                    //on modifie jour restant afin de se placer dans une configuration ou le prochain jour est un jour de congé et le premier de sa serie
                    if (choixShift == -1) {
                        //on ne veut pas enlever 17 jour off a la suite et faire croire qu'il reste beaucoup de jours disponible
                        jourManquant = -min(etatVariable[numJour][jourOffSuite], nbJourOffMinPersonne[numPersonne]);
                    }
                    else {
                        jourManquant = min(jourRestantApres, nbShiftSuccessifPersonne[numPersonne][1] - etatVariable[numJour][nbShiftSuite]);
                        tempsTravailRestantPossible += jourManquant * plusLongShift;
                    }

                    jourRestantApres -= jourManquant;
                    //cout << "jour manquant " << jourManquant << endl;


                    //nombre de fois que la sequence minimum de jour off - maximum de shift peut encore apparaitre
                    int nombreSequence = jourRestantApres / (nbShiftSuccessifPersonne[numPersonne][1] + nbJourOffMinPersonne[numPersonne]);
                    jourRestantApres = jourRestantApres % (nbShiftSuccessifPersonne[numPersonne][1] + nbJourOffMinPersonne[numPersonne]);

                    jourRestantApres -= nbJourOffMinPersonne[numPersonne];

                    if (jourRestantApres > 0) {
                        tempsTravailRestantPossible += jourRestantApres * plusLongShift;
                    }

                    tempsTravailRestantPossible += nombreSequence * nbShiftSuccessifPersonne[numPersonne][1] * plusLongShift;
                    //cout << "cas 1 : temps restant apres : " << nombreSequence * nbShiftSuccessifPersonne[numPersonne][1] * plusLongShift << endl;

                    //jour avant le debut
                    int jourRestantAvant = debut + 2;

                    //nombre de fois que la sequence minimum de jour off - maximum de shift peut encore apparaitre
                    nombreSequence = jourRestantAvant / (nbShiftSuccessifPersonne[numPersonne][1] + nbJourOffMinPersonne[numPersonne]);
                    jourRestantAvant = jourRestantAvant % (nbShiftSuccessifPersonne[numPersonne][1] + nbJourOffMinPersonne[numPersonne]);

                    jourRestantAvant -= nbJourOffMinPersonne[numPersonne];

                    if (jourRestantAvant > 0) {
                        tempsTravailRestantPossible += jourRestantAvant * plusLongShift;
                    }

                    tempsTravailRestantPossible += nombreSequence * nbShiftSuccessifPersonne[numPersonne][1] * plusLongShift;
                    //cout << "cas 1 : temps restant avant : " << nombreSequence * nbShiftSuccessifPersonne[numPersonne][1] * plusLongShift << endl;
                }
                else {
                    //jour avant le debut
                    jourRestant = debut - numJour - 1;

                    if (choixShift == -1) {
                        //on ne veut pas enlever 17 jour off a la suite et faire croire qu'il reste beaucoup de jours disponible
                        jourManquant = -min(etatVariable[numJour][jourOffSuite], nbJourOffMinPersonne[numPersonne]);
                    }
                    else {
                        jourManquant = min(jourRestant, nbShiftSuccessifPersonne[numPersonne][1] - etatVariable[numJour][nbShiftSuite]);
                        tempsTravailRestantPossible += jourManquant * plusLongShift;
                    }

                    jourRestant -= jourManquant;

                    //nombre de fois que la sequence minimum de jour off - maximum de shift peut encore apparaitre
                    int nombreSequence = jourRestant / (nbShiftSuccessifPersonne[numPersonne][1] + nbJourOffMinPersonne[numPersonne]);
                    jourRestant = jourRestant % (nbShiftSuccessifPersonne[numPersonne][1] + nbJourOffMinPersonne[numPersonne]);

                    jourRestant -= nbJourOffMinPersonne[numPersonne];

                    if (jourRestant > 0) {
                        tempsTravailRestantPossible += jourRestant * plusLongShift;
                    }

                    tempsTravailRestantPossible += nombreSequence * nbShiftSuccessifPersonne[numPersonne][1] * plusLongShift;
                    //cout << "cas 2 : temps restant : " << nombreSequence * nbShiftSuccessifPersonne[numPersonne][1] * plusLongShift << endl;
                }
                //cout << "tempsTravailRestantPossible" << tempsTravailRestantPossible << endl;
                //cout << "temps de travail " << etatVariable[numJour][tempsTravail] << endl;

                /*if (numPersonne == 10) {
                    cout << "jour restant " << jourRestant << endl;
                    cout << "jour manquant " << jourManquant << endl;
                }*/

                //nombre de fois que la sequence minimum de jour off - maximum de shift peut encore apparaitre


                /*if (numPersonne == 12) {
                    cout << "planning " << numPersonne << " : ";
                    for (int k = 0; k < nbJour; k++) {
                        cout << personne[k] << ", ";
                    }
                    cout << endl;
                }*/

                /*if (numPersonne == 12) {
                    cout << "etatVariable[i][tempsTravail] " << etatVariable[i][tempsTravail] << endl;
                    cout << "tempsTravailRestantPossible " << tempsTravailRestantPossible << endl;
                    cout << "i " << i << endl;
                    cout << "dureeTravailPersonne[numPersonne][0] " << dureeTravailPersonne[numPersonne][0] << endl;
                }*/

                if (etatVariable[numJour][tempsTravail] + tempsTravailRestantPossible < dureeTravailPersonne[numPersonne][0]) {
                    //cout << "pruned " << endl;

                    revenir = 1;
                    int choixShift = personne[numJour];
                    if (choixShift != -1) {
                        shiftRestant[choixShift]++;
                    }
                }
            }


            //check le temps minimum si le planning est plein
            /*if (i + 1 == nbJour) {
                if (etatVariable[nbJour - 1][tempsTravail] < dureeTravailPersonne[numPersonne][0]) {
                    cout << "valide" << endl;
                    revenir = 1;
                    int choixShift = personne[i];
                    if (choixShift != -1) {
                        //shiftRestant[choixShift]++;
                    }

                }
            }*/


            if (revenir) {
                //si l'algo a du mal a trouver une solution (il explore beaucoup de branche) on reset la solution de 0
                nbExploration++;
                if (nbExploration == maxExploration) {
                    //maxExploration *= 1;
                    //cout << "num personne " << numPersonne << ", " << maxExploration << endl;
                    solutionValide = false;
                    break;
                }

                //l'algo a invalidé trop de branche
                //debut + jour off min car les premiers jour sont des jours off
                if (numJour == (debut + nbJourOffMinPersonne[numPersonne]) % nbJour) {
                    //cout << "Aucune solution trouvé pour la personne " << numPersonne << endl;
                    solutionValide = false;
                    break;
                }

                int mauvaisChoix = personne[numVeille];
                mauvaisesBranche[numVeille].push_back(mauvaisChoix);


                if (mauvaisChoix != -1) {
                    shiftRestant[mauvaisChoix]++;
                }

                //if (numAvantVeille >= debut) { pas besoin de la condition : l'avant veille est forcement apres le debut
                etatVariable[numVeille][tempsTravail] = etatVariable[numAvantVeille][tempsTravail];
                etatVariable[numVeille][nbShiftSuite] = etatVariable[numAvantVeille][nbShiftSuite];
                etatVariable[numVeille][jourOffSuite] = etatVariable[numAvantVeille][jourOffSuite];
                etatVariable[numVeille][premierJourTravail] = etatVariable[numAvantVeille][premierJourTravail];
                etatVariable[numVeille][premierJourOff] = etatVariable[numAvantVeille][premierJourOff];
                etatVariable[numVeille][nbWE] = etatVariable[numAvantVeille][nbWE];

                i -= 2;
                continue;
            }

            //prepare la prochaine iteration
            if (numLendemain != debut) {

                //le chemin n'a jamais ete exploré, on le vide donc
                mauvaisesBranche[numLendemain].clear();

                //transmet ses variables a la prochaine iterations
                etatVariable[numLendemain][tempsTravail] = etatVariable[numJour][tempsTravail];
                etatVariable[numLendemain][nbShiftSuite] = etatVariable[numJour][nbShiftSuite];
                etatVariable[numLendemain][jourOffSuite] = etatVariable[numJour][jourOffSuite];
                etatVariable[numLendemain][premierJourTravail] = etatVariable[numJour][premierJourTravail];
                etatVariable[numLendemain][premierJourOff] = etatVariable[numJour][premierJourOff];
                etatVariable[numLendemain][nbWE] = etatVariable[numJour][nbWE];
            }
        }        
    }
    return 0;
}

void Algorithme::compter_erreur(vector<int>& resultat, vector<vector<int>>& planning) {
    vector<vector<vector<int>>> wrapper;
    wrapper.push_back(planning);
    compter_erreur(resultat, wrapper);
}

void Algorithme::compter_erreur(vector<int>& resultat, vector<vector<vector<int>>>& liste_planning) {

    for (int i = 0; i < liste_planning.size(); i++) {
        int erreur = 0;
        //on part du principe que personne n'a les shift qu'ils voulaient 
        //et on enleve le poid au score si la personne est affecté au shift qu'elle veut
        int score = sommePoidsVouloir;

        //erreur
        //verifier les sucessions de shift interdit
        // duree max de travail             (erreur = (temps travailé - temps max) / duree moyenne shift)
        // duree min de travail             (erreur = (temps min - temps travailé) / duree moyenne shift)
        // nb shift max a la suite          (1 erreur par jour de trop)
        // nb shift min a la suite          (1 erreur par jour manquant)
        // nb jour off consecutif min       (1 erreur par jour manquant)
        // nb max de we                     (1 erreur par we de trop)
        // nb max de shift                  (1 erreur par shift de trop)
        // congés                           (1 erreur par congé travaillé)
        // 
        // score
        // nombre de personne suplementaire par shift
        // nombre de personne manquante par shift
        //preference individuelle a vouloir un shift
        //preference individuelle a ne pas vouloir un shift

        vector<vector<int>>& planning = liste_planning[i];

        if (planning.size() != nbPersonne) {
            cout << "Le planning " << i << " ne contient pas assez de personne" << endl;
            cout << "taille planning " << planning.size() << endl;
            cout << "nb personne requise " << nbPersonne << endl;
            throw 1;
        }

        vector<vector<int>> nombrePersonneParShift(nbJour, vector<int>(nbShift, 0));
        for (int j = 0; j < nbPersonne; j++) {
            vector<int>& personne = planning[j];

            if (personne.size() != nbJour) {
                cout << "La personne " << j << " du planning " << i << " n'a pas tous ses jours definit" << endl;
                throw 1;
            }

            int dureeTravail = 0;
            int nbShiftSuccessif = 0;
            int nbJourOffSuccessif = 0;
            bool weTravaille = false;
            int nbWeTravaille = 0;
            bool premierJourOFF = false;
            bool premierJourOn = false;
            vector<int> shiftTravaille(nbShift, 0);

            for (int k = 0; k < nbJour; k++) {
                if (k % 7 == 0) weTravaille = false;

                int shift = personne[k];

                //test des shift ne pouvant se succeder
                if (shift != -1 && k != nbJour - 1) {
                    int suivant = personne[k + 1];
                    if (suivant != -1 && !shiftSuccInterdit[shift][suivant]) {
                        erreur++;
                        //cout << "erreur 1" << endl;
                    }
                }

                if (shift != -1) {
                    dureeTravail += dureeShift[shift];

                    //test du nombre de shift maximum consecutif
                    nbShiftSuccessif++;
                    if (nbShiftSuccessif > nbShiftSuccessifPersonne[j][1]) {
                        erreur++;
                        //cout << "erreur 2" << endl;
                    }

                    //test du nombre de jour off minimum
                    if (premierJourOn && nbJourOffSuccessif > 0 && nbJourOffSuccessif < nbJourOffMinPersonne[j]) {
                        erreur += nbJourOffMinPersonne[j] - nbJourOffSuccessif;
                        //cout << "erreur 3" << endl;
                        //cout << "personne " << j << endl;
                    }
                    nbJourOffSuccessif = 0;

                    //test nb we max travail
                    if ((k % 7 == 5 || k % 7 == 6) && !weTravaille) {
                        nbWeTravaille++;
                        weTravaille = true;
                    }

                    shiftTravaille[shift]++;

                    //test jour congés
                    if (jourConges[j][k]) {
                        erreur++;
                        //cout << "erreur 4" << endl;
                    }

                    //calcul score preference shift
                    score += poidsPasVouloirShift[j][k][shift];
                    score -= poidsVouloirShift[j][k][shift];

                    nombrePersonneParShift[k][shift] += 1;

                    premierJourOn = true;
                }
                else {
                    //test du nombre de shift minimum consecutif
                    if (premierJourOFF && nbShiftSuccessif > 0 && nbShiftSuccessif < nbShiftSuccessifPersonne[j][0]) {
                        erreur += nbShiftSuccessifPersonne[j][0] - nbShiftSuccessif;
                        //cout << "erreur 5" << endl;
                    }
                    nbShiftSuccessif = 0;

                    nbJourOffSuccessif++;

                    premierJourOFF = true;
                }
            }

            //test des duree min et max de travail
            if (dureeTravailPersonne[j][0] > dureeTravail) {
                erreur += (dureeTravailPersonne[j][0] - dureeTravail); // dureeMoyenneShift;
                /*cout << "erreur 6" << endl;
                cout << "duree " << dureeTravail << " de " << j << endl;
                cout << "planning " << j << " : ";
                for (int k = 0; k < nbJour; k++) {
                    cout << personne[k] << ", ";
                }
                cout << endl;*/

            }

            if (dureeTravail > dureeTravailPersonne[j][1]) {
                erreur += (dureeTravail - dureeTravailPersonne[j][1]); // dureeMoyenneShift;
                //cout << "erreur 7" << endl;
                //cout << "duree " << dureeTravail << " de " << j << endl;
            }

            //test du nombre max de we de travail
            if (nbWeTravaille > nbMaxWeTravailPersonne[j]) {
                erreur += nbWeTravaille - nbMaxWeTravailPersonne[j];
                //cout << "erreur 8" << endl;
            }

            //test du nombre max de shift
            for (int k = 0; k < nbShift; k++) {
                if (shiftTravaille[k] > nbShiftMax[j][k]) {
                    erreur += shiftTravaille[k] - nbShiftMax[j][k];
                    //cout << "erreur 9" << endl;
                }
            }
        }

        //calcul score nombre personne requis par shift
        for (int j = 0; j < nbJour; j++) {
            for (int k = 0; k < nbShift; k++) {
                //personne en moins
                if (nombrePersonneParShift[j][k] < nbPersonneRequisShift[j][k]) {
                    int difference = nbPersonneRequisShift[j][k] - nombrePersonneParShift[j][k];
                    score += difference * poidsPersonneManquante[j][k];
                }

                //personne en trop
                if (nombrePersonneParShift[j][k] > nbPersonneRequisShift[j][k]) {
                    int difference = nombrePersonneParShift[j][k] - nbPersonneRequisShift[j][k];
                    score += difference * poidsPersonneSurplus[j][k];
                }
            }
        }
        if (erreur) {
            /*
            cout << "planning " << i << endl;
            cout << "nombre erreur : " << erreur << endl;
            throw 1;
            */
            resultat[i] = -1;
        }
        else {
            resultat[i] = score;
        }
    }
}

void Algorithme::choisir_parent(vector<vector<vector<int>>>& parents, vector<vector<vector<int>>>& population, vector<int>& vect_nb_erreur) {

    /*// Associer chaque erreur avec sa solution dans un vecteur de paires
    vector<pair<int, int>> populations_erreurs(TAILLE_POPULATION);
    for (int i = 0; i < TAILLE_POPULATION; i++) {
        populations_erreurs[i] = { vect_nb_erreur[i], i };
    }

    // Trier les solutions en fonction du nombre d'erreurs (croissant)
    sort(populations_erreurs.begin(), populations_erreurs.end(), [](const pair<int, int>& a, const pair<int, int>& b) {
        return a.first < b.first;
        });


    // Sélection des meilleurs parents (moitié supérieure)
    for (int i = 0; i < TAILLE_POPULATION / 2; i++) {
        swap(population[populations_erreurs[i].second], parents[i]);
    }

    return;*/

    vector<pair<double, vector<vector<int>>&>> population_erreurs;

    // calcul du total d'erreurs 
    int total_erreurs = accumulate(vect_nb_erreur.begin(), vect_nb_erreur.end(), 0);
    
    //calcul de miminum d'erreur 
    //int indice = trouve_erreur_minimal(vect_nb_erreur);
    //int min_erreur = vect_nb_erreur[indice];

    int indice = trouve_erreur_maximal(vect_nb_erreur);
    int max_erreur = vect_nb_erreur[indice];

    // associer les solutions a leurs pourcentage d'erreurs 
    double cumule = 0;
    for (int i = 0; i < TAILLE_POPULATION; i++) {

        //(maxErreur - ierreur) / (maxErreur * TAILLEPOP - totalErreur) *100
        double pourcentage_erreur = (double)(max_erreur - vect_nb_erreur[i]) / (max_erreur * TAILLE_POPULATION - total_erreurs) * 100;

        //cout << "nb erreur : " << vect_nb_erreur[i] << ", proba : " << pourcentage_erreur << endl;
        
        //cout << "total erreur : " << total_erreurs << endl;


        population_erreurs.push_back({ cumule + pourcentage_erreur,population[i] });
        cumule += pourcentage_erreur;
        //cout << cumule << endl;
    }
    //cout << "min erreur : " << vect_nb_erreur[trouve_erreur_minimal(vect_nb_erreur)] << endl;
    //cout << "max erreur : " << max_erreur << endl;

    // trie du pourcentage par ordre croissant 
    /*sort(population_erreurs.begin(), population_erreurs.end(), [](const pair<double, Solution>& a, const pair<double, Solution>& b) {
        return a.first < b.first;
        });*/

    // construction de l'intervalle de sélection basé sur le pourcentage d'erreurs cumulés 
    /*vector<double> intervalle;
    double cumul = 0.0;
    for (const auto& elem : population_erreurs) {
        cumul += elem.first;
        intervalle.push_back(cumul);
    }*/

    //Générer les parents
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0, 100);  // Pourcentage entre 0 et 100

    for (int i = 0; i < TAILLE_POPULATION / 2; i++) {
        double a = dis(gen);
        // Trouver l'element qui correspond au pourcentage aléatoire 
        for (int j = 0; j < TAILLE_POPULATION; j++) {
            if (a <= population_erreurs[j].first) {
                parents[i] = population_erreurs[j].second;
                break;
            }
        }
    }
}

void Algorithme::genere_enfant(vector<vector<vector<int>>>& listeEnfants, vector<vector<vector<int>>>& listeParents) {
    // Parcourir les parents par paires
    for (int i = 0; i < listeParents.size() / 2; i++) {
        // Index pour les enfants générés
        int k = 2 * i; // Chaque paire de parents génère 4 enfants

        // Générer les enfants
        for (int j = 0; j < 2; j++) {
            operateurCroisement(listeEnfants[k+j], listeParents[2 * i], listeParents[2 * i + 1]);
        }
    }
}

void Algorithme::operateurCroisement(vector<vector<int>>& enfant, vector<vector<int>>& p1, vector<vector<int>>& p2) {

    // Créer un vecteur pour enregistrer quels indices proviennent de p1
    vector<bool> pris_de_p1(p1.size(), false);

    // Déterminer aléatoirement 50% des indices pour p1
    int half = p1.size() / 2;
    int count = 0;

    // Mélange aléatoire des indices
    vector<int> indices(p1.size());
    iota(indices.begin(), indices.end(), 0); // Remplit indices avec {0, 1, 2, ..., n-1}
    random_shuffle(indices.begin(), indices.end());

    // Sélectionner 50% des indices pour p1
    for (int i = 0; i < half; i++) {
        pris_de_p1[indices[i]] = true;
    }

    // Remplir l'enfant en choisissant à partir de p1 ou p2 selon pris_de_p1
    for (size_t i = 0; i < p1.size(); i++) {
        enfant[i] = pris_de_p1[i] ? p1[i] : p2[i];
    }
}

void Algorithme::appliquer_mutation(vector<vector<vector<int>>>& listeEnfants) {
    for (int j = 0; j < listeEnfants.size(); j++) {
        amelioreIndividu(listeEnfants[j]);
    }
}

void Algorithme::amelioreIndividu(vector<vector<int>>& individu) {
    vector<int> resultat(1);
    compter_erreur(resultat, individu);
    cout << "avant " << resultat[0] << endl;

    
    int ref = resultat[0];

    bool fini = false;
    while (!fini) {
        for (int personne = 0; personne < nbPersonne; personne++) {
            for (int jour = 0; jour < nbJour; jour++) {
                unsigned int minScore = -1;
                int shiftMinScore = individu[personne][jour];

                for (int shift = -1; shift < nbShift; shift++) {
                    individu[personne][jour] = shift;
                    compter_erreur(resultat, individu);

                    if (resultat[0] < minScore) {
                        minScore = resultat[0];
                        shiftMinScore = shift;
                    }
                }

                individu[personne][jour] = shiftMinScore;
            }
        }

        compter_erreur(resultat, individu);
        if (resultat[0] >= ref) {
            fini = true;
        }
        else {
            ref = resultat[0];
        }
    }
    cout << "apres " << resultat[0] << endl;

}

void Algorithme::updatePopulation(vector<vector<vector<int>>>& population, vector<vector<vector<int>>>& listeEnfant, vector<vector<vector<int>>>& listeParent) {
    std::swap_ranges(population.begin(), population.begin() + TAILLE_POPULATION / 2, listeParent.begin());
    std::swap_ranges(population.begin() + TAILLE_POPULATION / 2, population.begin() + TAILLE_POPULATION, listeEnfant.begin());
}

int Algorithme::trouve_erreur_minimal(const vector<int>& vect_nb_erreur) {
    // Vérifie si le vecteur est vide
    if (vect_nb_erreur.empty()) {
        throw std::invalid_argument("Le vecteur des erreurs est vide.");
    }

    // Trouver l'indice de la valeur minimale
    int min_erreur = vect_nb_erreur[0];
    int indice_min = 0;

    for (int i = 1; i < vect_nb_erreur.size(); i++) {
        if (vect_nb_erreur[i] < min_erreur) {
            min_erreur = vect_nb_erreur[i];
            indice_min = i;
        }
    }

    return indice_min; // Retourner l'indice de la solution avec l'erreur minimale
}
int Algorithme::trouve_erreur_maximal(const vector<int>& vect_nb_erreur) {
    // Vérifie si le vecteur est vide
    if (vect_nb_erreur.empty()) {
        throw std::invalid_argument("Le vecteur des erreurs est vide.");
    }

    // Trouver l'indice de la valeur minimale
    int min_erreur = vect_nb_erreur[0];
    int indice_min = 0;

    for (int i = 1; i < vect_nb_erreur.size(); i++) {
        if (vect_nb_erreur[i] > min_erreur) {
            min_erreur = vect_nb_erreur[i];
            indice_min = i;
        }
    }

    return indice_min; // Retourner l'indice de la solution avec l'erreur minimale
}