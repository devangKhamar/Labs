#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tuple>
#include <vector>
#include <algorithm>
#include <random>       // random number generators
#include <chrono>       // std::chrono::system_clock
#include <limits>       // upper bound of int
#include <iostream>
//configuration parameters for each run
#define MAXR 50
#define MAXP 70
#define MAXC 5000
#define BETA 0.9
#define FACTOR 1.1
#define W0 60000
#define T0 1000

bool tuple_compare (std::tuple <int,int> t1, std::tuple <int,int> t2){
  return (t1<=t2);
}

class ttp{

  int **dst_m;
  int **schedule, **schedule1 , **schf;
  int teams, rounds;
  double w;
  unsigned it = 0;
  unsigned INF;
  std::vector < std::tuple <int,int> > Q;
    // mt19937 is a standard mersenne_twister random number generator
  std::mt19937 mt;

  //private helper functions
  void read_DM(FILE*,int);
  int checkRR(int, int**);
  int find(std::vector<std::tuple<int,int>> , std::tuple<int,int>);
  void clearSchedule();
  int compute_cost(int**);
  int atmost(int**);
  int noRepeat(int**);
  double C(int**, int);

public:

  ttp(FILE*, int);
  int cost();
  //neighbourhood functions
  void swapHomes(int,int);
  void swapRounds(int,int);
  void swapTeams(int,int);
  void partialSwapRounds(int,int,int);
  void partialSwapTeams(int,int,int);
  void neighbourhood();
  //initial solution generators
  void randomSchedule();
  bool generateSchedule(std::vector < std::tuple <int,int> >, int**);
  //The Simulated Annealing algorithm
  int ttsa_optimization();
  //public helper function
  void printSchedule();
};


ttp::ttp(FILE *fi, int numTeams){
  //Contructor
  teams = numTeams;
  rounds = 2*numTeams - 2;
  read_DM(fi, teams);

  INF = std::numeric_limits<int>::max();
  schedule = (int**) calloc(teams, sizeof(int*));
  schedule1 = (int**) calloc(teams, sizeof(int*));
  schf = (int**) calloc(teams, sizeof(int*));
  for (int i = 0 ; i < teams; i++)
    {
      schedule[i] = (int*) calloc(rounds, sizeof(int));
      schedule1[i] = (int*) calloc(rounds, sizeof(int));
      schf[i] = (int*) calloc(rounds, sizeof(int));
    }
    w = W0;
    //Q.resize(teams*rounds);
}

void ttp::read_DM(FILE *fi, int size){
  // Helper function that reads distance matrix from the specified txt file.
  char c = '$';
    dst_m = (int**)malloc(size*sizeof(int*));
  for (int i = 0 ; i < size; i++)
    {
      dst_m[i] = (int*) malloc(size*sizeof(int));
      for(int j = 0; j < size; j++)
      {
        fscanf(fi," %d", &dst_m[i][j]);
      }
      while((c=getc(fi)) != '\n');
    }
}

int ttp::checkRR(int team, int** S){
  /*
  Helper function that verifies whether the schedule for a specific team satisfies
  hard constraints. It utilizes the fact that a double round robin schedule
  sums up to zero
  */
  int sum = 0;
  //index correction
  for (int rnd = 0; rnd < rounds; rnd++) {
    if(S[team-1][rnd] == 0){
      return 0;
    }
    sum += S[team-1][rnd];

  }
  if(sum == 0){
    return 1;
  }
  else{
    return 0;
  }
}

void ttp::clearSchedule(){
  //Helper function to set all entries fo schedule to zeros.
  for (int team = 0; team < teams; team++) {
    for (int rnd = 0; rnd < rounds; rnd++) {
        schedule[team][rnd] = 0;
    }
  }
}

int ttp::compute_cost(int **S){
  /*
  Helper function that computes cost in terms of total distance travelled
  by all teams throughout the tournament.
  */
  int cost = 0, rival = 0, prev_game = 1;
  for(int t= 0; t < teams; t++)
  { prev_game = t+1;
    for(int i = 0 ; i < rounds; i++)
    {
      rival = S[t][i];
      if(rival < 0)
       {
         rival *= -1;
          if (i == rounds-1){
            cost += dst_m[t][rival-1];
          }
          cost += dst_m[prev_game - 1][rival-1];
          prev_game = rival;
       }
       else{
         cost += dst_m[t][prev_game - 1];
         prev_game = t+1;
       }
     }
   }
  return cost;
}

int ttp::atmost(int** S){
  /*Tis function checks the soft constraint of having no more than three
    repeated home/away games. It returns the number of times this violation
    was made.
    */
  int home = 0, away = 0, violations = 0;
  for(int i = 0; i < teams; i++)
    {
      for (int j = 0 ; j < rounds; j++)
    	{
    	  if(S[i][j] < 0)
    	    {
    	      home++;
    	      away = 0;
    	    }
    	  else
    	    {
    	      home = 0;
    	      away++;
    	    }
    	  if((home > 3) || (away > 3))
    	    {
            violations++;
    	    }
    	}
      away = home = 0;
    }
  return violations;
}

int ttp::noRepeat(int **S){
  /* This function checks the soft constraint of no repeated back-to-back games.
    It returns the no. of times two teams compete with each other
    for two back to back rounds .
  */

  int violations = 0;
  for(int t = 0; t < teams; t++)
    {
      for(int r = 0 ; r < rounds - 1; r++)
	     {
	      if(abs(S[t][r]) == abs(S[t][r+1]))
	       {
            violations++;
	       }
      }
    }
  return violations;
}

int ttp::find(std::vector<std::tuple<int,int>> v, std::tuple<int,int> target){
  /*Helper function to find a specific tuple in a vector of tuples*/
  for(unsigned i = 0 ; i < v.size(); i++){
    if(v[i] == target){
      return i;
    }
  }
  return -1;
}

void ttp::printSchedule(){
  /*Helper function to print schdeule*/
  printf("\nTeam\\Round\n");
  for (int team = 0; team < teams; team++) {
    printf("%d:\t", team + 1);
    for (int rnd = 0; rnd < rounds; rnd++) {
      printf("%d\t", schedule[team][rnd]);
    }
    printf("\n");
    /* code */
  }
}

double ttp::C(int** S, int violations){
  /*
  Computes the cost of the schedule as a function of distance of travel
  and number of violations made by the current schedule. It penalizes violations
  using a sub-linear function.
  */
    double cost = 0 , penalty = 0;
    cost = compute_cost(S);

    if(violations == 0){
      return cost;
    }
    else{
      penalty = 1 + (sqrt(violations)*log(violations))/2; //the sub-linear function
      cost = sqrt((cost*cost) + (w*w*penalty*penalty));
    }
  return cost;
}

int ttp::cost(){
  return (C(schedule1, atmost(schedule1) + noRepeat(schedule1)));
}

void ttp::swapHomes(int team_i, int team_j){
  /*
  This function swaps the home and away games of two teams. If team_i
  plays at home with team_j in round k, then this function, would schedule
  team_i's game away at team_j's home in round k. A similar switch would be
  done for other home/away games. This function operates on a copy of the main
  schedule stored in schedule1.
  */
  int round_k = 0, round_l = 0,temp = 0,temp1 = 0;
  for (int rnd = 0; rnd < rounds; ++rnd)
    {
      if(schedule1[team_i - 1][rnd] == team_j)
	     {
	        round_k = rnd;
	       }
      else if(schedule1[team_i - 1][rnd] == -1*team_j)
	     {
	        round_l = rnd;
	     }
    }
  //swap
  temp = schedule1[team_i-1][round_k];
  schedule1[team_i - 1][round_k] = schedule1[team_i-1][round_l];
  schedule1[team_i-1][round_l] = temp;
  temp1 = schedule1[team_j-1][round_k];
  schedule1[team_j - 1][round_k] = schedule1[team_j-1][round_l];
  schedule1[team_j-1][round_l] = temp1;

}

void ttp::swapRounds(int round_k, int round_l){
  /*
  The swapRounds function swaps 2 rounds in the schedule. This is the same as
  swapping two coloumns in our schedule matrix. This function operates on
  schedule1 which is copy of the original schedule.
  */
  int temp = 0;
  for(int team = 0; team < teams; ++team)
    {
      //swap coloumn elements, row-by-row
      temp = schedule1[team][round_k - 1];
      schedule1[team][round_k - 1] = schedule1[team][round_l -1];
      schedule1[team][round_l - 1] = temp;
    }
}

void ttp::swapTeams(int team_i, int team_j){
  /*
  This function swaps all games of two teams (except the one against each other).
  It also performs the necessary corrections in each round, that must be made
  to ensure that the hard constarints of the schedule are satisfied. This
  function alters a copy of the main schedule stored in schedule1.
  */
  int temp = 0, t1 = 0;
  for (int rnd = 0; rnd < rounds; ++rnd) {

    if(abs(schedule1[team_i-1][rnd]) != abs(team_j)){
      // swap games in current round
      temp = schedule1[team_i-1][rnd];
      t1 = schedule1[team_j-1][rnd];
      schedule1[team_i-1][rnd] = t1;
      schedule1[team_j-1][rnd] = temp;
      // correct games of other teams in current round
      if(temp > 0){
          schedule1[temp-1][rnd] = -1*team_j;
      }
      else{
        schedule1[abs(temp)-1][rnd] = team_j;
      }
      if(t1 > 0){
        schedule1[t1 - 1][rnd] = -1*team_i;
      }
      else{
        schedule1[abs(t1) - 1][rnd] = team_i;
      }
    }
  }
}

void ttp::partialSwapRounds(int team, int round_i, int round_j) {
  /*
  The partialSwapRounds function swaps the games in two rounds for the
  specified team. This swap sets of an ejection chain which swaps games of
  other teams in this round. This is done to satisfy an the hard constraint
  of the schedule. This function alters a copy of the schedule stored in
  schedule1.
  */
  int temp = 0,valid = 0,rt = 0;
  bool swap = false,swapped[teams]={0};//index correction
  round_i --; round_j --; team --;
  int swaps = 0;

  temp = schedule1[team][round_j];
  schedule1[team][round_j] = schedule1[team][round_i];
  schedule1[team][round_i] = temp;
  swap = false;
  swapped[team]= 1;
  // swap corresponding elements
  valid = 0;
  while(!valid){
    //ejection chain.
      valid = 1;
      if(temp < 0){
        temp*=-1;
        if(schedule1[temp - 1][round_i] != team +1){
          swap = true;
          team = temp-1;
        }
      }
      else{
        if(schedule1[temp - 1][round_i] != -1*(team + 1)){
          swap = true;
          team = temp-1;
          }
      }
      if(swap &&!swapped[team]){
        valid = 0;
        temp = schedule1[team][round_j];
        schedule1[team][round_j] = schedule1[team][round_i];
        schedule1[team][round_i] = temp;
        swapped[team]= 1;
        swap = false;

        rt = round_j;
        round_j = round_i;
        round_i = rt;
        temp = schedule1[team][round_i];
        swaps++;
      }

  }
}

void ttp::partialSwapTeams(int team_i, int team_j ,int rnd){
  /*
  The partialSwapTeams function swaps the games of two teams in the specified
  round. This swap sets off an ejection chain that swaps games in the rest of
  the schedule to ensure that the schedule satisfies hard constraints.
  This function alters a copy of the original schedule stored in schedule1.
  */
  int temp = 0, settled = 1, t1 = 0, rcount = 0;
  int rOrg = rnd - 1;
  int switched[rounds] = {0};

  if(abs(schedule1[team_j-1][rnd-1]) == team_i){
    return;
  }
  else if(abs(schedule1[team_i-1][rnd-1]) == team_j){
    return;
  }

  temp = schedule1[team_j-1][rnd-1];
  t1 = schedule1[team_i-1][rnd-1];
  schedule1[team_j-1][rnd-1] = t1;
  schedule1[team_i-1][rnd-1] = temp;
  switched[rnd-1] = 1;
  if(temp > 0){
      schedule1[temp-1][rnd-1] = -1*team_i;
  }
  else{
    temp*= -1;
    schedule1[temp-1][rnd-1] = team_i;
    temp*= -1;
  }
  if(t1 > 0){
    schedule1[t1 - 1][rnd-1] = -1*team_j;
  }
  else{
    t1 *= - 1;
    schedule1[abs(t1) - 1][rnd-1] = team_j;
  }

  do{
    /*
    Implements ejection chains, swaps conflicts.We check for one team only,
    because RR for one implies RR for the other.
    */
    settled = 1;
    for (int r = 0; r < rounds; ++r) {
      // search all entries starting from the postion next to that of last swap.
      rcount = (r + (rnd)) % rounds;

      //find conflicting entries in the teams schedule
      if(schedule1[team_i-1][rcount] == temp){
        if(rcount == rOrg){
          continue;
        }
        settled = 0;
        rOrg = rcount;
        temp = schedule1[team_j-1][rcount];
        t1 = schedule1[team_i-1][rcount];
        schedule1[team_j-1][rcount] = t1;
        schedule1[team_i-1][rcount] = temp;
        if(temp > 0){
            schedule1[temp-1][rcount] = -1*team_i;
        }
        else{
          temp *= -1;
          schedule1[temp-1][rcount] = team_i;
          temp *= -1;
        }
        if(t1 > 0){
          schedule1[t1 - 1][rcount] = -1*team_j;
        }
        else{
          t1 *= -1;
          schedule1[t1 - 1][rcount] = team_j;
        }
        //printf("\n%d\tpost edit\n", (r+rnd)%rounds);
      }//endif
    }//endfor
  }while(!settled);//endwhile
}

void ttp::randomSchedule(){
  /* This function initialzies the list containing tuples of (team,week). It
  then invokes the recursive generateSchedule() function to construct the
  initial schedule. The schedule generated needs to be verfied for hard
  constraint (double-round robin) satisfaction. This is done by calling
  the checkRR() function. generateSchdeule() can return incomplete schedules
  because it was not able to assign teams with the randomized choice list
  created, hence multiple instances of the function may have to be run.
  */
  //initialzie Q
  int index = 0, valid = 0, cost_l = INF, c;

  std::vector < std::tuple <int,int> > Qt;

  Q.resize(teams*rounds);
  // initialze the (team,week) list
  for (int team = 0; team < teams; ++team) {
    for(int rnd = 0; rnd< rounds; ++rnd){
      std::get<0>(Q[index]) = team+1;
      std::get<1>(Q[index]) = rnd+1;
      index++;
    }
  }

  //sort the listlexicographically.
  std::sort(Q.begin(), Q.end());
  Qt.resize(Q.size());
  // create a copy of the list to avoid regeneration.
  Qt = Q;
  for(int i = 0; i < 4 ; i++)
    {
      do{
      Q = Qt;
      generateSchedule(Q,schedule);
        valid = 1;
        for(int team = 1 ; team <= teams; ++team){
          if(!checkRR(team, schedule)){
            valid = 0;
            // clear the invalid schedule
            clearSchedule();
            break;
          }
        }
      }while(!valid);
      //copy new schedule to schedule1.
      c = cost();
      if(cost_l > c){
        cost_l = c;
        for (int team = 0; team < teams; ++team) {
          for(int rnd = 0; rnd< rounds; ++rnd){
            schedule1[team][rnd] = schedule[team][rnd];
          }
        }
        clearSchedule();
      }
    }

    for (int team = 0; team < teams; ++team) {
      for(int rnd = 0; rnd< rounds; ++rnd){
        schedule[team][rnd] = schedule1[team][rnd];
      }
    }
}

bool ttp::generateSchedule(std::vector < std::tuple <int,int> > q , int** scheduleT){
  /*
  This function implments the recursion needed to arrive at an initial solution
  via backtracking. It recursively calls itself when it finds that for the
  selected team, the rival has not been assigned in the selected week. A copy
  of the (team,week) list (q), is passed to the called function. The original
  list is only updated if function returns true, indicating that assignment
  attempted in the current iteration is acceptable. Otherwise, it tries another
  configuration. The (team,week) pairs are stored as tuples, and all pairs are
  stored as a vector of tuples.
  */
  if(q.size() == 0){
    //Q.resize(0);
    return true;
  }

  std::vector < std::tuple <int,int> > Qt;
  std::tuple <int,int> local(0,0),chosen1(0,0);
  int o = 0,w = 0,t = 0,id = 0, alloted = 0;
  std::vector <int> Choices;

  std::sort(q.begin(),q.end(),tuple_compare);
  chosen1 = q[0];
  t = std::get<0>(chosen1);
  w = std::get<1>(chosen1);
  std::get<1>(local) = w;

  for(int i = 1 ; i <= teams ; i++){
    if(i==t){
      continue;
    }
      Choices.push_back(i);
      Choices.push_back(-1*i);
  }



  //uses a custom RNG with a new seed for every run.
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle (Choices.begin(),Choices.end(),std::default_random_engine(seed));

  //shuffling ensures that the order in which the rivals are assigned is random.

    for (unsigned choice = 0; choice < Choices.size(); choice++) {
      /* code */

      o = Choices[choice];
      std::get<0>(local) = abs(o);
      if(find(q,local) != - 1){
        alloted = 0;
        for(int rnd = 0; rnd < rounds; rnd++){
          if(scheduleT[t-1][rnd] == o){
            alloted = 1;
            break;
          }
        }
        if(!alloted){
          scheduleT[t-1][w-1] = o;
        }
        else{
          continue;
        }

        if(o > 0){
            for(int rnd = 0; rnd < rounds; rnd++){
            if(scheduleT[o-1][rnd] == -1*t){
              alloted = 1;
              break;
            }
          }
          if(!alloted){
            scheduleT[o-1][w-1] = -1*t;
          }
          else{
            continue;
          }
        }
    else{
        for(int rnd = 0; rnd < rounds; rnd++){
        if(scheduleT[abs(o)-1][rnd] == t){
          alloted = 1;
          break;
          }
        }
      //scheduleb[1] = scheduleT[o-1][w-1];
        if(!alloted){
          scheduleT[abs(o)-1][w-1] = t;
        }
        else{
          continue;
        }
      }

        std::get<0>(local) = abs(o);
        //update set
        Qt.resize(q.size());
        Qt = q;
        id = find(q,local);
        if(id!= -1){
          Qt.erase(Qt.begin() + id);
        }
        id = find(q,chosen1);
        if(id!= -1){
          Qt.erase(Qt.begin() + id);
        }
        Q.resize(Qt.size());
        Q = Qt;
        if(generateSchedule(Qt, scheduleT)){
          q.resize(Q.size());
          q = Q;
          return true;
        }
      }//endif
    }//endfor
  return false;
}

void ttp::neighbourhood(){
  /*
  The heart of TTSA's exploration. This function randomly selects
  a move among the 5 moves that allow TTSA to search for optimal solutions
  in it's neighbourhood. This function is slightly dissimilar to the one
  suggested in the paper. This function is biased towards the partialSwap
  moves. The motivation behind this was that, the partialSwap moves permit
  exploration of a larger search space (O(n^3) vs O(n^2)), hence we reduce
  run time by increasing the probability of finding an optimal solution early.

  The neighbourhood function uses a mersenne_twister random number generator.
  This is a psuedo random number generator, which can generate numbers at
  high speeds. Additionally, the engine can be tweaked to UNIFORMLY generate
  numbers in a specified range. Using rand() % range_upperBound, destroys the
  statistical distribution of numbers generated and requires more iterations
  to explore the solution space.

  Additionally rand() is dangerous! :https://channel9.msdn.com/Events/GoingNative/2013/rand-Considered-Harmful
  */
  enum move {SwapTeams ,SwapHomes, SwapRounds, PartialSwapRounds, PartialSwapTeams=6};
  int choice = 0, ti =0 ,tj = 0,ri = 0 ,rj = 0;

  // restrict the bounds of numbers generated.
  std::uniform_int_distribution<int> dist(0,8);
  std::uniform_int_distribution<int> t(1,teams);
  std::uniform_int_distribution<int> r(1,rounds);

  choice = dist(mt);

  for (int team = 0; team < teams; ++team) {
    for(int rnd = 0; rnd< rounds; ++rnd){
      schedule1[team][rnd] = schedule[team][rnd];
    }
  }
  //generate team and round numbers.
  ti = (t(mt)); tj = (t(mt));
  ri = (r(mt)); rj = (r(mt));

    while(ti==tj){
    tj = t(mt);     //ensuring that we don't chose moves that have no affect
  }
  while(ri==rj){
    rj = r(mt);
  }
  switch(choice)
  { //bias is implmented by assigning more cases to the partialSwap functions
    case SwapHomes: swapHomes(ti,tj);
        break;
    case SwapRounds: swapRounds(ri,rj);
        break;
    case SwapTeams: swapTeams(ti,tj);
        break;
    case PartialSwapRounds:
    case 4:
    case 5: partialSwapRounds(ti,ri,rj);
        break;
    case PartialSwapTeams:
    case 7:
    case 8: partialSwapTeams(ti,tj,rj);
        break;
    default:
      break;
  }

}

int ttp::ttsa_optimization(){
  /*
  This implements the TTSA algorithm.
  It computers number of violations,
    at-> no. of "atmost 3 home/away games" constraint violation
    nr-> no. of "no repeated games" constraint violation
  cost of each modified schdeule and stores each new valid schedule
  generated in schf. This ifed back to the schedule right before the
  function returns
  */
  unsigned bestFeasible = INF , nbf = INF;
  unsigned bestInfeasible = INF,nbi = INF;
  unsigned reheat = 0, counter = 0, phase = 0;
  unsigned nbv = 0, nbv1 = 0, at = 0, nr = 0;;
  double costS, costS1, deltaCost = 0;
  bool accept = false, valid = false;
  double T = T0, bestTemp = T0;
  double imd = 0;

  while(reheat <= MAXR){
    phase = 0;
    printf("\nreheat:%d ", reheat);
    //printSchedule();
    while(phase <= MAXP){
      counter = 0;
      //printf("\nphase:%d ", phase);
      while(counter <= MAXC){
        //neighbourhood operates on a copy of the schedule by default;
        neighbourhood();
        at = atmost(schedule);
        nr = noRepeat(schedule);
        nbv = at + nr;
        at = atmost(schedule1);
        nr = noRepeat(schedule1);
        nbv1 = at + nr;
        costS1 = C(schedule1, nbv1);
        costS = C(schedule, nbv);
        if((costS1 < costS) ||
        (nbv1 == 0 && costS1 < bestFeasible) ||
        (nbv1 > 0 && costS1 < bestInfeasible))
        {
          accept = true;
        }
        else{
          deltaCost = costS1 - costS;
          imd = exp((double) (-1*deltaCost)/T);
          if( imd > 0.5){
              accept = true;
          }
          else{
            accept = false;
          }
        }//endif
        if(accept){
          //S <- S'
          for (int t = 0; t < teams; ++t) {
            for (int r = 0; r < rounds; ++r) {
              schedule[t][r] = schedule1[t][r];
            }//endfor
          }//endfor
          if(nbv1 == 0){
            nbf = (costS1 < bestFeasible) ? costS1 : bestFeasible;
          }
          else{
            nbi = (costS1 < bestInfeasible) ? costS1 : bestInfeasible;
          }//endif
          if((nbf < bestFeasible) || (nbi < bestInfeasible)){
            // reset params if new best value found.
            reheat = 0; counter = 0; phase = 0;
            bestTemp = T;
            bestFeasible = nbf;
            bestInfeasible = nbi;

            if (nbv1 == 0){
              valid = 1;
              w = w/FACTOR;
              //store the new valid schedule, the main copy gets updated later
              for (int t = 0; t < teams; ++t) {
                for (int r = 0; r < rounds; ++r) {
                  schf[t][r] = schedule[t][r];
                }//endfor
              }//endfor
            }
            else{
              w = w*FACTOR;
            }//endif nbv_
          }
          else{
            counter++;
          }
        }//endif
      }//while !count
      phase++;
      T = T*BETA;
    }//while !phase
    reheat ++;
    T = 2*bestTemp;
  }//while !reheat

  //check whether a valid schedule was actually obtained
  if(!valid){
    printf("\n NO VALID Schedule FOUND!\n");
    printf("\n best bestInfeasible: %d \n", nbi);
    fflush(stdout);
    }
    else{
      printf("\n Valid schdeule Found!");
    //copy best feasible schedule found so far into final schedule matrix.
    for (int t = 0; t < teams; ++t) {
      for (int r = 0; r < rounds; ++r) {
        schedule[t][r] = schf[t][r];
      }//endfor
    }//endfor
  }


  return nbf;
}


int main(){/*
  Where The Magic Happens! The main function opens the file containing The
  travel distances and passes the file pointer and the size of the problem
  instance (number of teams), to the constructor object. It then invokes
  randomSchedule to generate the initial greedy solution obtained by
  backtracking. This solution is latter used by the ttsa_optimization
  algorithm to generate a better schedule. Once the optimization is complete,
  main() recives the cost of the optimized schedule, prints the optimized
  schedule to cli and prints total travel distance for the optimized schedule.
  */
  int costf = 0;
  FILE *fi;

  //Change name of file for different instances
  fi = fopen("data/data12.txt","r");
  if(fi == 0)
    {
      printf("\n Unable to open file. Program terminated");
      exit(-1);
    }

   ttp inst(fi,12);
   inst.randomSchedule();
   printf("\nInitialized...");
   costf = inst.ttsa_optimization();
   inst.printSchedule();

   fclose(fi);
   printf("\n Distance: %d", costf);

  return 0;
}

// *************************** END OF CODE ************************************
