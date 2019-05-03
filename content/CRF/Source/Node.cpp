#include "../Header/Node.h"
#include <time.h>
using namespace std;

//#include <iostream> //only for debugging
namespace Segugio {

	template<typename T>
	void exist_in_list(bool* result, const list<T>& L, const T& candidate) {

		*result = false;
		for (auto it = L.begin(); it != L.end(); it++) {
			if (*it == candidate) {
				*result = true;
				return;
			}
		}

	};






	void Node::Neighbour_connection::Recompute_Neighboorhoods(list<Neighbour_connection*>& connections) {

		auto it = connections.begin();
		for (it; it != connections.end(); it++)
			(*it)->Neighbourhood.clear();

		list<Neighbour_connection*> processed;
		list<Neighbour_connection*>::iterator it2;
		for (it = connections.begin(); it != connections.end(); it++) {
			(*it)->Neighbourhood = processed;

			for (it2 = processed.begin(); it2 != processed.end(); it2++)
				(*it2)->Neighbourhood.push_back(*it);

			processed.push_back(*it);
		}

	}





	Node::~Node() {

		for (auto it = this->Temporary_Unary.begin(); it != this->Temporary_Unary.end(); it++)
			delete *it;

		for (auto it = this->Permanent_Unary.begin(); it != this->Permanent_Unary.end(); it++)
			delete *it;

		for (auto it = this->Active_connections.begin(); it != this->Active_connections.end(); it++)
			delete *it;

		for (auto it = this->Disabled_connections.begin(); it != this->Disabled_connections.end(); it++)
			delete *it;

		delete this->pVariable;

	}

	void Node::Gather_all_Unaries(std::list<Potential*>* result) {

		result->clear();

		this->Append_temporary_permanent_Unaries(result);

		for (auto it = this->Active_connections.begin(); it != this->Active_connections.end(); it++) {
#ifdef _DEBUG
			if ((*it)->Message_to_this_node == NULL) {
				system("ECHO Found NULL incoming messages not computed");
				abort();
			}
#endif // _DEBUG
			result->push_back((*it)->Message_to_this_node);
		}

#ifdef _DEBUG
		if (result->empty()) {
			system("ECHO empty set of messages found");
			abort();
		}
#endif // _DEBUG

	}

	void Node::Append_temporary_permanent_Unaries(list<Potential*>* result) {

		auto it = this->Permanent_Unary.begin();
		for (it; it != this->Permanent_Unary.end(); it++)
			result->push_back(*it);

		for (it = this->Temporary_Unary.begin(); it != this->Temporary_Unary.end(); it++)
			result->push_back(*it);

	}

	void Node::Append_permanent_Unaries(std::list<Potential*>* result) {

		auto it = this->Permanent_Unary.begin();
		for (it; it != this->Permanent_Unary.end(); it++)
			result->push_back(*it);

	}

	void Node::Compute_neighbour_set(std::list<Node*>* Neigh_set) {

		Neigh_set->clear();
		for (auto itn = this->Active_connections.begin(); itn != this->Active_connections.end(); itn++)
			Neigh_set->push_back((*itn)->Neighbour);

	}

	void Node::Compute_neighbour_set(std::list<Node*>* Neigh_set, std::list<Potential*>* binary_in_Neigh_set) {

		Neigh_set->clear();
		binary_in_Neigh_set->clear();
		for (auto itn = this->Active_connections.begin(); itn != this->Active_connections.end(); itn++) {
			Neigh_set->push_back((*itn)->Neighbour);
			binary_in_Neigh_set->push_back((*itn)->Shared_potential);
		}

	}

	void Node::Compute_neighbourhood_messages(std::list<Potential*>* messages, Node* node_involved_in_connection) {

		messages->clear();

		this->Append_temporary_permanent_Unaries(messages);

		for (auto it = this->Active_connections.begin(); it != this->Active_connections.end(); it++) {
			if ((*it)->Neighbour == node_involved_in_connection) {
				for (auto it_N = (*it)->Neighbourhood.begin(); it_N != (*it)->Neighbourhood.end(); it_N++) {
#ifdef _DEBUG
					if ((*it_N)->Message_to_this_node == NULL) {
						system("ECHO Found NULL incoming messages not computed");
						abort();
					}
#endif // _DEBUG
					messages->push_back((*it_N)->Message_to_this_node);
				}
				return;
			}
		}

		system("ECHO inexsistent neighbour node");
		abort();


	}





	Node::Node_factory::~Node_factory() {

		for (auto itN = this->Nodes.begin(); itN != this->Nodes.end(); itN++)
			delete *itN;

		for (auto itP = this->Binary_potentials.begin(); itP != this->Binary_potentials.end(); itP++)
			delete *itP;

		if (this->Last_propag_info != NULL) delete this->Last_propag_info;

	}

	void Node::Node_factory::Set_Iteration_4_belief_propagation(const unsigned int& iter_to_use) {

		if (iter_to_use < 10) {
			system("ECHO iterations for belief propagation are too low");
			abort();
		}

		this->Iterations_4_belief_propagation = iter_to_use;

	}

	Categoric_var* Find_by_name(list<Categoric_var*>& vars, const string& name) {

		for (auto it = vars.begin(); it != vars.end(); it++) {
			if ((*it)->Get_name().compare(name) == 0)
				return *it;
		}
		return NULL;

	};

	Potential_Shape* Import_shape(const string& prefix, XML_reader::Tag_readable& tag, const list<Categoric_var*>& vars) {

		Potential_Shape* shape = new Potential_Shape(vars);

		if (tag.Exist_Field("Source"))  shape->Import(prefix + tag.Get_value("Source"));
		else {
			list<XML_reader::Tag_readable> distr_vals;
			tag.Get_Nested("Distr_val", &distr_vals);
			list<string> indices_raw;
			list<size_t> indices;
			while (!distr_vals.empty()) {
				indices.clear();
				distr_vals.front().Get_values_specific_field_name("v", &indices_raw);

				while (!indices_raw.empty()) {
					indices.push_back((size_t)atoi(indices_raw.front().c_str()));
					indices_raw.pop_front();
				}

				shape->Add_value(indices, (float)atof(distr_vals.front().Get_value("D").c_str()));
				distr_vals.pop_front();
			}
		}

		return shape;

	};
	void Node::Node_factory::Import_from_XML(XML_reader* reader, const std::string& prefix_config_xml_file) {

		auto root = reader->Get_root();

		//import variables
		list<Categoric_var*>	variables;

		Categoric_var* temp_clone;
		list<XML_reader::Tag_readable> Var_tags;
		root.Get_Nested("Variable", &Var_tags);
		string var_name;
		for (auto itV = Var_tags.begin(); itV != Var_tags.end(); itV++) {
			var_name = itV->Get_value("name");
			temp_clone = Find_by_name(variables, var_name);

			if (temp_clone != NULL) {
				system("ECHO found clone of a varibale when parsing xml graph");
				abort();
			}

			variables.push_back(new Categoric_var((size_t)atoi(itV->Get_value("Size").c_str()), var_name));

		}

		//import potentials
		struct pote_info {
			Potential_Shape*		shape;
			float					weight; //-1.f for pure shape function not to wrap in a log model
			list<Categoric_var*>	vars;
		};

		list<XML_reader::Tag_readable> Pot_tags;
		root.Get_Nested("Potential", &Pot_tags);
		list<string> var_names;

		list<pote_info> bin_pot;
		list<pote_info> una_pot;
		for (auto itP = Pot_tags.begin(); itP != Pot_tags.end(); itP++) {
			itP->Get_values_specific_field_name("var", &var_names);
			if (var_names.size() == 1) { //new unary potential to read
				una_pot.push_back(pote_info());
				una_pot.back().weight = -1.f;
				temp_clone = Find_by_name(variables, var_names.front());
				if (temp_clone == NULL) {
					system("ECHO potential refers to an inexistent variable");
					abort();
				}
				una_pot.back().vars.push_back(temp_clone);
				una_pot.back().shape = Import_shape(prefix_config_xml_file, *itP, una_pot.back().vars);
				if (itP->Exist_Field("weight")) {
					una_pot.back().weight = (float)atof(itP->Get_value("weight").c_str());
					if (una_pot.back().weight < 0.f) {
						system("ECHO weight of potential must be positive");
						abort();
					}
				}
			}
			else if (var_names.size() == 2) { //new binary potential to read
				bin_pot.push_back(pote_info());
				bin_pot.back().weight = -1.f;
				temp_clone = Find_by_name(variables, var_names.front());
				if (temp_clone == NULL) {
					system("ECHO potential refers to an inexistent variable");
					abort();
				}
				bin_pot.back().vars.push_back(temp_clone);
				temp_clone = Find_by_name(variables, var_names.back());
				if (temp_clone == NULL) {
					system("ECHO potential refers to an inexistent variable");
					abort();
				}
				bin_pot.back().vars.push_back(temp_clone);
				bin_pot.back().shape = Import_shape(prefix_config_xml_file, *itP, bin_pot.back().vars);
				if (itP->Exist_Field("weight")) {
					bin_pot.back().weight = (float)atof(itP->Get_value("weight").c_str());
					if (bin_pot.back().weight < 0.f) {
						system("ECHO weight of potential must be positive");
						abort();
					}
				}
			}
			else {
				system("ECHO valid potentials must refer only to 1 or 2 variables");
				abort();
			}
		}

		if (bin_pot.empty()) {
			system("ECHO at least one binary potential must be present in a config file");
			abort();
		}

		//add binary potentials
		if (bin_pot.front().weight == -1.f)
			this->Insert(bin_pot.front().shape);
		else
			this->Insert(new Potential_Exp_Shape(bin_pot.front().shape, bin_pot.front().weight));

		list<Categoric_var*> processed_vars = bin_pot.front().vars;
		bin_pot.pop_front();
		list<pote_info>::iterator it_bin;
		bool bA, bB;
		while (!bin_pot.empty()) {
			//find a binary potential to insert
			for (it_bin = bin_pot.begin(); it_bin != bin_pot.end(); it_bin++) {
				exist_in_list(&bA, processed_vars, it_bin->vars.front());
				exist_in_list(&bB, processed_vars, it_bin->vars.back());
				if (bA || bB) {

					if (bin_pot.front().weight == -1.f)
						this->Insert(it_bin->shape);
					else
						this->Insert(new Potential_Exp_Shape(it_bin->shape, it_bin->weight));

					break;
				}
			}

			if (it_bin == bin_pot.end()) {
				system("graph found in the config file is not unique");
				abort();
			}


			if (!bA) processed_vars.push_back(it_bin->vars.front());
			if (!bB) processed_vars.push_back(it_bin->vars.back());
			bin_pot.erase(it_bin);
		}

		//add unary potentials
		for (auto itU = una_pot.begin(); itU != una_pot.end(); itU++) {
			if (itU->weight == -1.f)
				this->Insert(itU->shape);
			else
				this->Insert(new Potential_Exp_Shape(itU->shape, itU->weight));
		}

		if (processed_vars.size() != variables.size()) {
			system("ECHO some variables declared in the config file were not included in any potentials");
			abort();
		}

	}


	void Node::Node_factory::Insert(Potential* pot, Categoric_var* varA, Categoric_var* varB) {

		Node* peer_A = NULL;
		Node* peer_B = NULL;

		auto itN = this->Nodes.begin();
		for (itN; itN != this->Nodes.end(); itN++) {
			if ((*itN)->Get_var() == varA) {
				peer_A = *itN;
				break;
			}
		}
		itN = this->Nodes.begin();
		for (itN; itN != this->Nodes.end(); itN++) {
			if ((*itN)->Get_var() == varB) {
				peer_B = *itN;
				break;
			}
		}

		if ((peer_A != NULL) && (peer_B != NULL)) {
			//check this potential was not already inserted
			const list<Categoric_var*>* temp_var;
			for (auto it_bb = this->Binary_potentials.begin();
				it_bb != this->Binary_potentials.end(); it_bb++) {
				temp_var = (*it_bb)->Get_involved_var_safe();

				if (((peer_A->Get_var() == temp_var->front()) && (peer_B->Get_var() == temp_var->back())) ||
					((peer_A->Get_var() == temp_var->back()) && (peer_B->Get_var() == temp_var->front()))) {
					system("ECHO found clone of an already inserted binary potential");
					abort();
				}
			}
		}

		if (peer_A == NULL) {
			this->Nodes.push_back(new Node(varA));
			peer_A = this->Nodes.back();
		}
		if (peer_B == NULL) {
			this->Nodes.push_back(new Node(varB));
			peer_B = this->Nodes.back();
		}

		//create connection
		Node::Neighbour_connection* A_B = new Node::Neighbour_connection();
		A_B->This_Node = peer_A;
		A_B->Neighbour = peer_B;
		A_B->Shared_potential = pot;
		A_B->Message_to_this_node = NULL;

		Node::Neighbour_connection* B_A = new Node::Neighbour_connection();
		B_A->This_Node = peer_B;
		B_A->Neighbour = peer_A;
		B_A->Shared_potential = pot;
		B_A->Message_to_this_node = NULL;

		A_B->Message_to_neighbour_node = &B_A->Message_to_this_node;
		B_A->Message_to_neighbour_node = &A_B->Message_to_this_node;

		peer_A->Active_connections.push_back(A_B);
		peer_B->Active_connections.push_back(B_A);

		this->Binary_potentials.push_back(pot);

	}

	void Node::Node_factory::Insert(Potential* pot, Categoric_var* varU) {

		auto itN = this->Nodes.begin();
		for (itN; itN != this->Nodes.end(); itN++) {
			if ((*itN)->Get_var() == varU) {
				(*itN)->Permanent_Unary.push_back(pot);
				return;
			}
		}

		system("ECHO the unary potential provided refers to an inexistent node");
		abort();

	}



	Categoric_var* Node::Node_factory::Find_Variable(const std::string& var_name) {

		return this->Find_Node(var_name)->pVariable;

	}

	Node* Node::Node_factory::Find_Node(const std::string& var_name) {

		for (auto it = this->Nodes.begin(); it != this->Nodes.end(); it++) {
			if ((*it)->Get_var()->Get_name().compare(var_name) == 0)
				return *it;
		}

		system("ECHO Node::Node_factory::Find_Node, inexistent name");
		abort();

	}

	void Node::Node_factory::Set_Observation_Set_var(const std::list<Categoric_var*>& new_observed_vars) {

		if (this->mState == 0) {
			this->mState = 1;
		}
		/*else if (this->mState = 1) {

		}*/
		else if (this->mState == 2) {
			if (new_observed_vars.empty() && this->Last_observation_set.empty()) {
				return;
			}

			if (this->Last_propag_info != NULL) delete this->Last_propag_info;
			this->Last_propag_info = NULL;
			this->mState = 1;
		}

		auto itN = this->Nodes.begin();
		list<Neighbour_connection*>::iterator it_neigh;
		for (itN; itN != this->Nodes.end(); itN++) {
			for (it_neigh = (*itN)->Disabled_connections.begin(); it_neigh != (*itN)->Disabled_connections.end(); it_neigh++)
				(*itN)->Active_connections.push_back(*it_neigh);
			(*itN)->Disabled_connections.clear();
		}

		bool not_managed;
		list<Neighbour_connection*>::iterator it_neigh2;
		this->Last_observation_set.clear();
		for (auto it_var = new_observed_vars.begin(); it_var != new_observed_vars.end(); it_var++) {
			not_managed = true;
			for (itN = this->Nodes.begin(); itN != this->Nodes.end(); itN++) {
				if ((*itN)->pVariable == *it_var) {
					for (it_neigh = (*itN)->Active_connections.begin(); it_neigh != (*itN)->Active_connections.end(); it_neigh++) {
						(*itN)->Disabled_connections.push_back(*it_neigh);

						for (it_neigh2 = (*it_neigh)->Neighbour->Active_connections.begin();
							it_neigh2 != (*it_neigh)->Neighbour->Active_connections.end(); it_neigh2++) {
							if ((*it_neigh2)->Neighbour == *itN) {
								(*it_neigh)->Neighbour->Disabled_connections.push_back(*it_neigh2);
								it_neigh2 = (*it_neigh)->Neighbour->Active_connections.erase(it_neigh2);
								break;
							}
						}
					}
					(*itN)->Active_connections.clear();

					this->Last_observation_set.push_back(observation_info());
					this->Last_observation_set.back().Involved_node = *itN;

					not_managed = false;
					break;
				}
			}

			if (not_managed) {
				system("ECHO inexistent variable observed");
				abort();
			}
		}

		//recompute all neighbourhood
		for (itN = this->Nodes.begin(); itN != this->Nodes.end(); itN++)
			Neighbour_connection::Recompute_Neighboorhoods((*itN)->Active_connections);

		this->Recompute_clusters();

	}

	void Node::Node_factory::Set_Observation_Set_val(const std::list<size_t>& new_observed_vals) {

		if (this->mState == 0) {
			system("ECHO you cannot set values before defining observation set");
			abort();
		}
		else if (this->mState == 1) {
			this->mState = 2;
		}
		else if (this->mState == 2) {
			if (this->Last_propag_info != NULL) delete this->Last_propag_info;
			this->Last_propag_info = NULL;
		}


		if (new_observed_vals.size() != this->Last_observation_set.size()) {
			system("ECHO Inconsistent number of observations");
			abort();
		}

		//delete all previous temporary messages
		list<Potential*>::iterator it_temporary;
		for (auto itN = this->Nodes.begin(); itN != this->Nodes.end(); itN++) {
			for (it_temporary = (*itN)->Temporary_Unary.begin(); 
				it_temporary != (*itN)->Temporary_Unary.end(); it_temporary++) {
				delete *it_temporary;
			}
			(*itN)->Temporary_Unary.clear();
		}

		auto it_val = new_observed_vals.begin();
		Potential* message_reduced;
		list<Neighbour_connection*>::iterator it_conn;
		for (auto it = this->Last_observation_set.begin(); it != this->Last_observation_set.end(); it++) {
			it->Value = *it_val;

			//compute the temporary messages produced by this observation
			for (it_conn = it->Involved_node->Disabled_connections.begin(); it_conn != it->Involved_node->Disabled_connections.end(); it_conn++) {
				message_reduced = new Potential({ it->Value }, { it->Involved_node->pVariable }, (*it_conn)->Shared_potential);
				(*it_conn)->Neighbour->Temporary_Unary.push_back(message_reduced);
			}

			it_val++;
		}

	}

	void Node::Node_factory::Get_Actual_Hidden_Set(std::list<Categoric_var*>* result) {

#ifdef _DEBUG
		if (this->mState == 0) {
			abort();
		}
#endif // DEBUG

		result->clear();
		list<Node*>::iterator itN;
		for (auto itC = this->Last_hidden_clusters.begin(); itC != this->Last_hidden_clusters.end(); itC++) {
			for (itN = itC->begin(); itN != itC->end(); itN++)
				result->push_back((*itN)->pVariable);
		}

	}

	void Node::Node_factory::Get_Actual_Observation_Set(std::list<Categoric_var*>* result) {

#ifdef _DEBUG
		if (this->mState == 0) {
			abort();
		}
#endif // DEBUG

		result->clear();
		for (auto it = this->Last_observation_set.begin(); it != this->Last_observation_set.end(); it++)
			result->push_back(it->Involved_node->pVariable);

	}

	void Node::Node_factory::Get_All_variables_in_model(std::list<Categoric_var*>* result) {

		result->clear();
		for (auto it = this->Nodes.begin(); it != this->Nodes.end(); it++) {
			result->push_back((*it)->pVariable);
		}

	}



	void Node::Node_factory::Belief_Propagation(const bool& sum_or_MAP) {

		if (this->Nodes.empty()) {
			system("ECHO empty structure");
			abort();
		}

		if (this->mState == 0) {
			this->Set_Observation_Set_var({});
			this->Set_Observation_Set_val({});
			this->mState = 2;
		}
		else if (this->mState == 1) {
			system("ECHO You tried to make belief propoagation before setting observations");
			abort();
		}
		else if (this->mState == 2) {
			if (this->Belief_Propagation_Redo_checking(sum_or_MAP)) return;
		}

		//cout << "new propagation \n";

	//reset all messages for nodes in the hidden set
		auto it_cl = this->Last_hidden_clusters.begin();
		list<Node*>::iterator it_it_cl;
		list<Neighbour_connection*>::iterator it_conn;
		for (it_cl; it_cl != this->Last_hidden_clusters.end(); it_cl++) {
			for (it_it_cl = it_cl->begin(); it_it_cl != it_cl->end(); it_it_cl++) {
				for (it_conn = (*it_it_cl)->Active_connections.begin(); it_conn != (*it_it_cl)->Active_connections.end(); it_conn++) {
					if ((*it_conn)->Message_to_this_node != NULL) {
						delete (*it_conn)->Message_to_this_node;
						(*it_conn)->Message_to_this_node = NULL;
					}
				}
			}
		}

     //perform belief propagation
		bool all_done_within_iter = true;
		for (it_cl = this->Last_hidden_clusters.begin(); it_cl != this->Last_hidden_clusters.end(); it_cl++) {
			if (!I_belief_propagation_strategy::Propagate(*it_cl, sum_or_MAP, this->Iterations_4_belief_propagation))
				all_done_within_iter = false;
		}

		if (this->Last_propag_info == NULL) this->Last_propag_info = new last_belief_propagation_info();
		this->Last_propag_info->Iterations_perfomed = this->Iterations_4_belief_propagation;
		this->Last_propag_info->Terminate_within_iter = all_done_within_iter;
		this->Last_propag_info->Last_was_SumProd_or_MAP = sum_or_MAP;

	}

	bool Node::Node_factory::Belief_Propagation_Redo_checking(const bool& sum_or_MAP) {

		if (this->Last_propag_info != NULL) {
			if (this->Last_propag_info->Last_was_SumProd_or_MAP == sum_or_MAP) {
				if (this->Last_propag_info->Terminate_within_iter) return true;
				else if (this->Last_propag_info->Iterations_perfomed == this->Iterations_4_belief_propagation) return true;
			}
		}
		
		return false;

	}

	void Node::Node_factory::Recompute_clusters() {

		this->Last_hidden_clusters.clear();
		list<Node*> open_set = this->Nodes;
		list<Node*>::iterator it_cluster;
		list<Node*>::iterator it_neigh;
		list<Node*> temp_neigh;
		bool temp;
		while (!open_set.empty()) {
			this->Last_hidden_clusters.push_back(list<Node*>());

			this->Last_hidden_clusters.back().push_back(open_set.front());
			open_set.pop_front();
			it_cluster = this->Last_hidden_clusters.back().begin();
			while (it_cluster != this->Last_hidden_clusters.back().end()) {

				(*it_cluster)->Compute_neighbour_set(&temp_neigh);

				for (it_neigh = temp_neigh.begin(); it_neigh != temp_neigh.end(); it_neigh++) {

					exist_in_list(&temp, this->Last_hidden_clusters.back(), *it_neigh);

					if (!temp) {
						this->Last_hidden_clusters.back().push_back(*it_neigh);
						open_set.remove(*it_neigh);
					}
				}
				it_cluster++;
			}
		}

		auto itt = this->Last_hidden_clusters.begin();
		list<observation_info>::iterator it_last;
		while (itt != this->Last_hidden_clusters.end()) {
			if (itt->size() == 1) {
				temp = false;
				for (it_last = this->Last_observation_set.begin(); it_last != this->Last_observation_set.end(); it_last++) {
					if (it_last->Involved_node == itt->front()) {
						temp = true;
						break;
					}
				}

				if (temp) itt = this->Last_hidden_clusters.erase(itt);
				else itt++;
			}
			else itt++;
		}

	}

	void Node::Node_factory::Get_marginal_distribution(std::list<float>* result, Categoric_var* var) {

		this->Belief_Propagation(true);

		list<Node*>::iterator itN;
		for (auto it_cluster = this->Last_hidden_clusters.begin(); it_cluster != this->Last_hidden_clusters.end(); it_cluster++) {
			for (itN = it_cluster->begin(); itN != it_cluster->end(); itN++) {
				if ((*itN)->Get_var() == var) {
					list<Potential*> messages_union;
					(*itN)->Gather_all_Unaries(&messages_union);
					Potential P(messages_union, false);
					P.Get_marginals(result);
					return;
				}
			}
		}

		system("ECHO You Asked to inference for a variable not in the hidden set");
		abort();

	}

	void Node::Node_factory::MAP_on_Hidden_set(std::list<size_t>* result) {

		result->clear();

		this->Belief_Propagation(false);

		list<float> marginals;
		list<float>::iterator it_m;
		float max;
		size_t k;

		list<Potential*> messages_union;
		list<Node*>::iterator itN;
		for (auto it = this->Last_hidden_clusters.begin(); it != this->Last_hidden_clusters.end(); it++) {
			for (itN = it->begin(); itN != it->end(); itN++) {
				(*itN)->Gather_all_Unaries(&messages_union);
				Potential P(messages_union, false);
				P.Get_marginals(&marginals);

				//find values maximising marginals
				result->push_back(0);
				it_m = marginals.begin();
				max = *it_m;
				it_m++;
				k = 1;
				for (it_m; it_m != marginals.end(); it_m++) {
					if (*it_m > max) {
						max = *it_m;
						result->back() = k;
					}
					k++;
				}
			}
		}

	}

	void sample_from_discrete(size_t* result, const list<float>& distr) {

		float r = (float)rand() / (float)RAND_MAX;

		auto it = distr.begin();
		if (r <= *it) {
			*result = 0;
			return;
		}

		float cum = *it;
		it++;
		size_t k = 1;
		for (it; it != distr.end(); it++) {
			cum += *it;
			if (r <= cum) {
				*result = k;
				return;
			}
			k++;
		}

		*result = (distr.size() - 1);
		return;

	}
	struct info_neighbourhood {
		struct info_neigh {
			Potential*     shared_potential;
			Categoric_var* Var;
			size_t	       Var_pos;
		};

		size_t              Involved_var_pos;
		list<info_neigh>	Info;
		list<Potential*>	Unary_potentials;
	};
	void Evolve_samples(size_t* X, list<info_neighbourhood>& Infoes, const int& iterations) {

		list<Potential*> temp;
		list<Potential*>::iterator it_temp;
		list<info_neighbourhood::info_neigh>::iterator it_info;
		list<float> marginal;
		size_t original_pot;
		auto it = Infoes.begin();

		for (int k = 0; k < iterations; k++) {
			for (it = Infoes.begin(); it != Infoes.end(); it++) {
				temp = it->Unary_potentials;
				original_pot = temp.size();

				for (it_info = it->Info.begin(); it_info != it->Info.end(); it_info++)
					temp.push_back(new  Potential({ X[it_info->Var_pos] }, { it_info->Var }, it_info->shared_potential));

				Potential temp_pot(temp, false);
				temp_pot.Get_marginals(&marginal);

				sample_from_discrete(&X[it->Involved_var_pos], marginal);

				it_temp = temp.begin();
				advance(it_temp, original_pot);
				for (it_temp; it_temp != temp.end(); it_temp++)
					delete *it_temp;
			}
		}

	};
	void Node::Node_factory::Gibbs_Sampling_on_Hidden_set(std::list<std::list<size_t>>* result, const unsigned int& N_samples, const unsigned int& initial_sample_to_skip) {

		srand((unsigned int)time(NULL));

		result->clear();

		this->Belief_Propagation(true);

		list<info_neighbourhood> Infoes;

		list<Node*>				 neigh_temp;
		list<Node*>::iterator    neigh_temp_it;
		list<Potential*>		 neigh_bin_temp;
		list<Potential*>::iterator neigh_bin_temp_it;
		size_t k;

		list<Node*>::iterator itN;
		list<Categoric_var*> hidden_vars;
		this->Get_Actual_Hidden_Set(&hidden_vars);
		list<Categoric_var*>::iterator itN2;
		for (auto itcl = this->Last_hidden_clusters.begin(); itcl != this->Last_hidden_clusters.end(); itcl++) {
			for (itN = itcl->begin(); itN != itcl->end(); itN++) {
				Infoes.push_back(info_neighbourhood());

				k = 0;
				for (itN2 = hidden_vars.begin(); itN2 != hidden_vars.end(); itN2++) {
					if (*itN2 == (*itN)->pVariable) {
						Infoes.back().Involved_var_pos = k;
						break;
					}
					k++;
				}
				(*itN)->Append_temporary_permanent_Unaries(&Infoes.back().Unary_potentials);

				(*itN)->Compute_neighbour_set(&neigh_temp, &neigh_bin_temp);


				neigh_bin_temp_it = neigh_bin_temp.begin();
				for (neigh_temp_it = neigh_temp.begin(); neigh_temp_it != neigh_temp.end(); neigh_temp_it++) {
					Infoes.back().Info.push_back(info_neighbourhood::info_neigh());

					Infoes.back().Info.back().shared_potential = *neigh_bin_temp_it;
					Infoes.back().Info.back().Var = (*neigh_temp_it)->pVariable;
					k = 0;
					for (itN2 = hidden_vars.begin(); itN2 != hidden_vars.end(); itN2++) {
						if (*itN2 == (*neigh_temp_it)->pVariable) {
							Infoes.back().Info.back().Var_pos = k;
							break;
						}
						k++;
					}
					neigh_bin_temp_it++;
				}
			}
		}

		////////////////////// debug
		//for (auto iitt = Infoes.begin(); iitt != Infoes.end(); iitt++) {
		//	cout << iitt->Involved_var_pos << " ";
		//	for (auto iitt2 = iitt->Info.begin(); iitt2 != iitt->Info.end(); iitt2++) {
		//		cout << iitt2->Var_pos << " ";
		//	}

		//	cout << endl;
		//}
		//////////////////

		size_t hidded_set_size = this->Nodes.size() - this->Last_observation_set.size();

		size_t* X = (size_t*)malloc(sizeof(size_t)*hidded_set_size);
		for (k = 0; k < hidded_set_size; k++)
			X[k] = 0;

		//skip initial samples
		unsigned int n;
		unsigned int n_delta = (int)floorf(initial_sample_to_skip*0.1f);
		if (n_delta == 0) n_delta = 1;

		Evolve_samples(X, Infoes, initial_sample_to_skip);

		for (n = 0; n < N_samples; n++) {
			Evolve_samples(X, Infoes, n_delta);
			result->push_back(list<size_t>());
			for (k = 0; k < hidded_set_size; k++)
				result->back().push_back(X[k]);
		}

		free(X);

	}

	size_t* Node::Node_factory::Get_observed_val_in_case_is_in_observed_set(Categoric_var* var) {

		for (auto it = this->Last_observation_set.begin(); it != this->Last_observation_set.end(); it++) {
			if (it->Involved_node->Get_var() == var) return &it->Value;
		}

		return NULL;
		
	}

}