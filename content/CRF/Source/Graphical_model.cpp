#include "../Header/Graphical_model.h"
using namespace std;

namespace Segugio {

	Graph::Graph(const std::string& config_xml_file, const std::string& prefix_config_xml_file) {

		XML_reader reader(prefix_config_xml_file + config_xml_file);
		this->Import_from_XML(&reader, prefix_config_xml_file); 

	};






	I_Learning_handler::I_Learning_handler(Potential_Exp_Shape* pot_to_handle) : I_Potential_Decorator(pot_to_handle) , ref_to_wrapped_exp_potential(pot_to_handle) {

		this->Destroy_wrapped = false; //this exponential shape will be wrapped also by a Potential to be stored in the graphical model
		this->pWeight = this->Handler_weight::Get_weight(pot_to_handle);

		list<size_t*> val_to_search;
		Get_entire_domain(&val_to_search, *this->I_Potential::Get_involved_var(pot_to_handle));
		Find_Comb_in_distribution(&this->Extended_shape_domain, val_to_search, *this->Get_involved_var(), this->Get_shape());
		for (auto it = val_to_search.begin(); it != val_to_search.end(); it++)
			free(*it);

	}

	I_Learning_handler::I_Learning_handler(I_Learning_handler* other) :
		I_Learning_handler(other->ref_to_wrapped_exp_potential) {  };

	void I_Learning_handler::Get_grad_alfa_part(float* alfa, std::list<size_t*>* comb_in_train_set, std::list<Categoric_var*>* comb_var) {

		list<I_Distribution_value*> val_in_train_set;
		Find_Comb_in_distribution(&val_in_train_set, *comb_in_train_set, *comb_var, this->Get_shape());

		*alfa = 0.f;
		float temp;
		for (auto it_val = val_in_train_set.begin(); it_val != val_in_train_set.end(); it_val++) {
			if (*it_val != NULL) {
				(*it_val)->Get_val(&temp);
				*alfa += temp;
			}
		}
		*alfa *= 1.f / (float)comb_in_train_set->size();

	}

	void Dot_with_Prob(float* result, const list<float>& marginal_prob, const list<I_Potential::I_Distribution_value*>& shape) {

		*result = 0.f;
		float temp;
		auto itP = marginal_prob.begin();
		for (auto itD = shape.begin(); itD != shape.end(); itD++) {
			if (*itD != NULL) {
				(*itD)->Get_val(&temp);
				*result += temp * *itP;
			}

			itP++;
		}

	}

	void I_Learning_handler::Cumul_Log_Activation(float* result, size_t* val_to_consider, const list<Categoric_var*>& var_in_set) {

		list<Potential::I_Distribution_value*> matching;
		this->Find_Comb_in_distribution(&matching, list<size_t*>({ val_to_consider }), var_in_set, this->ref_to_wrapped_exp_potential);
		float temp;

#ifdef _DEBUG
		if (matching.front() == NULL) {
			abort();
		}
#endif // _DEBUG

		matching.front()->Get_val(&temp);
		*result += logf(temp);

	}



	class Unary_handler : public I_Learning_handler {
	public:
		Unary_handler(Node* N, Potential_Exp_Shape* pot_to_handle) : I_Learning_handler(pot_to_handle), pNode(N) {};
	private:
		void    Get_grad_beta_part(float* beta);
	// data
		Node*				pNode;
	};

	void Unary_handler::Get_grad_beta_part(float* beta) {

		list<float> marginals;
		list<Potential*> message_union;
		this->pNode->Gather_all_Unaries(&message_union);
		Potential UP(message_union);
		UP.Get_marginals(&marginals);

		Dot_with_Prob(beta , marginals, this->Extended_shape_domain);

	}



	class Binary_handler : public I_Learning_handler {
	public:
		Binary_handler(Node* N1, Node* N2, Potential_Exp_Shape* pot_to_handle);
		~Binary_handler() { delete this->Binary_for_group_marginal_computation; };
	private:
		void    Get_grad_beta_part(float* beta);
	// data
		Node*				pNode1;
		Node*				pNode2;
	// cache
		Potential*						Binary_for_group_marginal_computation;
	};

	Binary_handler::Binary_handler(Node* N1, Node* N2, Potential_Exp_Shape* pot_to_handle) : I_Learning_handler(pot_to_handle), pNode1(N1), pNode2(N2) {

		if (N1->Get_var() != pot_to_handle->Get_involved_var_safe()->front()) {
			Node* C = N2;
			N2 = N1;
			N1 = C;
		}

		auto temp = new Potential_Shape(*this->Get_involved_var());
		temp->Set_ones();
		this->Binary_for_group_marginal_computation = new Potential(temp);

	};

	void Binary_handler::Get_grad_beta_part(float* beta) {

		list<Potential*> union_temp;

		struct info_val {
			Potential*					 Mex_tot;
			list<I_Distribution_value*>  ordered_distr;
			size_t						 var_involved;
		};
		list<info_val> Messages_from_Net;

		list<info_val> infoes;
		this->pNode1->Compute_neighbourhood_messages(&union_temp, this->pNode2);
		if (!union_temp.empty()) {
			Messages_from_Net.push_back(info_val());
			Messages_from_Net.back().Mex_tot = new Potential(union_temp);
			Messages_from_Net.back().var_involved = 0;
			list<size_t*> dom_temp;
			Get_entire_domain(&dom_temp, { this->Binary_for_group_marginal_computation->Get_involved_var_safe()->front() });
			Find_Comb_in_distribution(&Messages_from_Net.back().ordered_distr, dom_temp, 
				{ this->Binary_for_group_marginal_computation->Get_involved_var_safe()->front() }, Messages_from_Net.back().Mex_tot);
			for (auto it = dom_temp.begin(); it != dom_temp.end(); it++)
				free(*it);
		}
		this->pNode2->Compute_neighbourhood_messages(&union_temp, this->pNode1);
		if (!union_temp.empty()) {
			Messages_from_Net.push_back(info_val());
			Messages_from_Net.back().Mex_tot = new Potential(union_temp);
			Messages_from_Net.back().var_involved = 1;
			list<size_t*> dom_temp;
			Get_entire_domain(&dom_temp, { this->Binary_for_group_marginal_computation->Get_involved_var_safe()->back() });
			Find_Comb_in_distribution(&Messages_from_Net.back().ordered_distr, dom_temp,
				{ this->Binary_for_group_marginal_computation->Get_involved_var_safe()->back() }, Messages_from_Net.back().Mex_tot);
			for (auto it = dom_temp.begin(); it != dom_temp.end(); it++)
				free(*it);
		}

		auto pBin_Distr = this->Get_distr();
		float temp, result;
		list<I_Distribution_value*>::iterator it_pos;
		list<info_val>::iterator it_info;
		auto itD2 = I_Potential::Get_distr(this->Binary_for_group_marginal_computation)->begin();
		for (auto itD = pBin_Distr->begin(); itD != pBin_Distr->end(); itD++) {
			(*itD)->Get_val(&result);

			for (it_info = infoes.begin(); it_info != infoes.end(); it_info++) {
				it_pos = it_info->ordered_distr.begin();
				advance(it_pos, (*itD)->Get_indeces()[it_info->var_involved]);

				(*it_pos)->Get_val(&temp);
				result *= temp;
			}

			(*itD2)->Set_val(result);
			itD2++;
		}

		list<float> Marginals;
		this->Binary_for_group_marginal_computation->Get_marginals(&Marginals);

		Dot_with_Prob(beta, Marginals, this->Extended_shape_domain);

		for (it_info = infoes.begin(); it_info != infoes.end(); it_info++)
			delete it_info->Mex_tot;

	}






	Graph_Learnable::~Graph_Learnable() {

		for (auto it = this->Model_handlers.begin(); it != this->Model_handlers.end(); it++)
			delete *it;

	}

	void Graph_Learnable::Insert(Potential_Exp_Shape* pot) {
		
		this->Node_factory::Insert(pot);

		auto vars = pot->Get_involved_var_safe();

		if (vars->size() == 1) {
			//new unary
			this->Model_handlers.push_back(new Unary_handler(this->Find_Node(vars->front()->Get_name()), pot));
		}
		else {
			//new binary
			Node* N1 = this->Find_Node(vars->front()->Get_name());
			Node* N2 = this->Find_Node(vars->back()->Get_name());

			this->Model_handlers.push_back(new Binary_handler(N1, N2, pot));
		}

	}

	void Graph_Learnable::Insert(Potential_Shape* pot) {

		system("ECHO you can insert a pure shape function only to general graph and not (Conditional) Random Field ");
		abort();

	}

	void Graph_Learnable::Weights_Manager::Get_w(std::list<float>* w, Graph_Learnable* model) {

		w->clear();
		for (auto it = model->Model_handlers.begin(); it != model->Model_handlers.end(); it++) {
			w->push_back(float());
			(*it)->Get_weight(&w->back());
		}

	}

	void Graph_Learnable::Weights_Manager::Get_w_grad(std::list<float>* grad_w, Graph_Learnable* model, std::list<size_t*>* comb_train_set, std::list<Categoric_var*>* comb_var_order) {

		grad_w->clear();
		model->Graph_Learnable::Get_w_grad(grad_w, comb_train_set, comb_var_order); //here tha alfa part is appended
		model->Get_w_grad(grad_w, comb_train_set, comb_var_order); //here the beta part is added
		model->pLast_train_set = comb_train_set;

		//add regularization term
		float temp;
		auto it_grad = grad_w->begin();
		for (auto it = model->Model_handlers.begin(); it != model->Model_handlers.end(); it++) {
			(*it)->Get_weight(&temp);
			*it_grad -= 2.f * temp;
			it_grad++;
		}

	}

	void Graph_Learnable::Weights_Manager::Set_w(const std::list<float>& w, Graph_Learnable* model) {

		auto itw = w.begin();
		for (auto it = model->Model_handlers.begin(); it != model->Model_handlers.end(); it++) {
			(*it)->Set_weight(*itw);
			itw++;
		}

	}

	void Graph_Learnable::Get_w_grad(std::list<float>* grad_w, std::list<size_t*>* comb_train_set, std::list<Categoric_var*>* comb_var_order) {

		if (comb_train_set != this->pLast_train_set) {
			//recompute alfa part
			this->Alfa_part_gradient.clear();

			for (auto it = this->Model_handlers.begin(); it != this->Model_handlers.end(); it++) {
				this->Alfa_part_gradient.push_back(float());
				(*it)->Get_grad_alfa_part(&this->Alfa_part_gradient.back(), comb_train_set, comb_var_order);
			}
		}

		for (auto it = this->Alfa_part_gradient.begin(); it != this->Alfa_part_gradient.end(); it++)
			grad_w->push_back(*it);

	}

	void Graph_Learnable::Get_Log_activation(float* result, size_t* Y, std::list<Categoric_var*>* Y_var_order) {

		*result = 0.f;
		for (auto it = this->Model_handlers.begin(); it != this->Model_handlers.end(); it++)
			(*it)->Cumul_Log_Activation(result, Y, *Y_var_order);

	}





	Random_Field::Random_Field(const std::string& config_xml_file, const std::string& prefix_config_xml_file) {

		XML_reader reader(prefix_config_xml_file + config_xml_file);
		this->Import_from_XML(&reader, prefix_config_xml_file);

	};

	void Random_Field::Get_w_grad(std::list<float>* grad_w, std::list<size_t*>* comb_train_set, std::list<Categoric_var*>* comb_var_order) {

		this->Set_Observation_Set_var(list<Categoric_var*>());
		this->Set_Observation_Set_val(list<size_t>());
		this->Belief_Propagation(true);

		float temp;
		auto it_grad = grad_w->begin();
		for (auto it = this->Model_handlers.begin(); it != this->Model_handlers.end(); it++) {
			(*it)->Get_grad_beta_part(&temp);
			*it_grad -= temp;
			it_grad++;
		}

	}

	void Random_Field::Get_Likelihood_estimation(float* result, std::list<size_t*>* comb_train_set, std::list<Categoric_var*>* comb_var_order) {

		this->Set_Observation_Set_var(list<Categoric_var*>());
		this->Set_Observation_Set_val(list<size_t>());
		list<size_t> Y_MAP;
		this->MAP_on_Hidden_set(&Y_MAP);
		size_t* Y_MAP_malloc = (size_t*)malloc(sizeof(size_t)*Y_MAP.size());
		size_t k = 0;
		for (auto it = Y_MAP.begin(); it != Y_MAP.end(); it++) {
			Y_MAP_malloc[k] = *it;
			k++;
		}

		list<Categoric_var*> MAP_order;
		this->Get_Actual_Hidden_Set(&MAP_order);

		float MAP_activation; 
		this->Get_Log_activation(&MAP_activation, Y_MAP_malloc, &MAP_order);

		*result = -MAP_activation * (float)comb_train_set->size();
		float temp;
		for (auto it_set = comb_train_set->begin(); it_set != comb_train_set->end(); it_set++) {
			this->Get_Log_activation(&temp, *it_set, comb_var_order);
			*result += temp;
		}

		free(Y_MAP_malloc);

	}



	class Binary_handler_with_Observation : public I_Learning_handler {
	public:
		Binary_handler_with_Observation(Node* Hidden_var, size_t* observed_val, I_Learning_handler* handle_to_substitute);
	private:
		void    Get_grad_beta_part(float* beta);
	// data
		Node*				pNode_hidden;
		size_t*				ref_to_val_observed;
	};

	Binary_handler_with_Observation::Binary_handler_with_Observation(Node* Hidden_var, size_t* observed_val, I_Learning_handler* handle_to_substitute) :
		I_Learning_handler(handle_to_substitute), pNode_hidden(Hidden_var), ref_to_val_observed(observed_val) {

		delete handle_to_substitute;

	};

	void Binary_handler_with_Observation::Get_grad_beta_part(float* beta) {

		list<float> marginals;
		list<Potential*> message_union;
		this->pNode_hidden->Gather_all_Unaries(&message_union);
		Potential UP(message_union);
		UP.Get_marginals(&marginals);

		size_t pos_hidden = 0, pos_obsv = 1;
		if (this->Get_involved_var()->back() == this->pNode_hidden->Get_var()) {
			pos_hidden = 1;
			pos_obsv = 0;
		}

		list<size_t*> comb_to_search;
		for (size_t k = 0; k < this->pNode_hidden->Get_var()->size(); k++) {
			comb_to_search.push_back((size_t*)malloc(sizeof(size_t) * 2));
			comb_to_search.back()[pos_hidden] = k;
			comb_to_search.back()[pos_obsv] = *this->ref_to_val_observed;
		}
		list<I_Distribution_value*> distr_conditioned_to_obsv;
		Find_Comb_in_distribution(&distr_conditioned_to_obsv, comb_to_search, *this->Get_involved_var(), this->Get_shape());

		Dot_with_Prob(beta, marginals, distr_conditioned_to_obsv);

		for (auto it = comb_to_search.begin(); it != comb_to_search.end(); it++)
			free(*it);

	}




	Conditional_Random_Field::Conditional_Random_Field(const std::string& config_xml_file, const std::string& prefix_config_xml_file) {

		XML_reader reader(prefix_config_xml_file + config_xml_file);
		this->Import_from_XML(&reader, prefix_config_xml_file);

		//read the observed set
		list<string> observed_names;
		XML_reader::Tag_readable root = reader.Get_root();
		list<XML_reader::Tag_readable> vars;
		root.Get_Nested("Variable", &vars);
		string flag;
		for (auto it = vars.begin(); it != vars.end(); it++) {
			if (it->Exist_Field("flag")) {
				flag = it->Get_value("flag");
				if (flag.compare("O") == 0)
					observed_names.push_back(it->Get_value("name"));
			}
		}

		if (observed_names.empty()) {
			system("ECHO Found CRF with no observed variables");
			abort();
		}

		list<Categoric_var*> observed_vars;
		for (auto it = observed_names.begin(); it != observed_names.end(); it++)
			observed_vars.push_back(this->Find_Variable(*it));

		this->Node_factory::Set_Observation_Set_var(observed_vars);

		list<size_t*> temp;
		list<Categoric_var*>::const_iterator it_temp;
		auto it_hnd = this->Model_handlers.begin();
		while(it_hnd != this->Model_handlers.end()){
			temp.clear();
			auto vars = (*it_hnd)->Get_involved_var_safe();
			for (it_temp = vars->begin(); it_temp != vars->end(); it_temp++)
				temp.push_back(this->Get_observed_val_in_case_is_in_observed_set(*it_temp));

			if (temp.size() == 1) {
				if (temp.front() != NULL) {
					//remove this unary handler since is attached to an observation
					delete *it_hnd;
					it_hnd = this->Model_handlers.erase(it_hnd);
					system("ECHO warning: detected redundant potentials attached to observed variables");
				}
				else it_hnd++;
			}
			else {
				if ((temp.front() != NULL) && (temp.back() != NULL)) {
					//remove this binary handler since is attached to an observation
					delete *it_hnd;
					it_hnd = this->Model_handlers.erase(it_hnd);
					system("ECHO warning: detected redundant potentials attached to observed variables");
				}
				else if (temp.front() != NULL) {
					*it_hnd = new Binary_handler_with_Observation(this->Find_Node(vars->back()->Get_name()), temp.front(), *it_hnd);
					it_hnd++;
				}
				else if (temp.back() != NULL) {
					*it_hnd = new Binary_handler_with_Observation(this->Find_Node(vars->front()->Get_name()), temp.back(), *it_hnd);
					it_hnd++;
				}
				else it_hnd++;
			}
		}

	};

	void extract_observations(std::list<size_t>* result, size_t* entire_vec, const std::list<size_t>& var_pos) {

		result->clear();
		for (auto it = var_pos.begin(); it != var_pos.end(); it++)
			result->push_back(entire_vec[*it]);

	}

	void find_observed_order(list<size_t>* result, const std::list<Categoric_var*>& order_in_model, const std::list<Categoric_var*>& order_in_train_set) {

		result->clear();

		list<Categoric_var*>::const_iterator it_in_train_set;
		size_t k;
		for (auto it = order_in_model.begin(); it != order_in_model.end(); it++) {
			k = 0;
			for (it_in_train_set = order_in_train_set.begin(); it_in_train_set != order_in_train_set.end(); it_in_train_set++) {
				if (*it_in_train_set == *it) {
					result->push_back(k);
					break;
				}
				k++;
			}
		}

	}

	void Conditional_Random_Field::Get_w_grad(std::list<float>* grad_w, std::list<size_t*>* comb_train_set, std::list<Categoric_var*>* comb_var_order) {

		list<size_t> pos_of_observed_var;
		list<Categoric_var*> observed_var_temp;
		this->Get_Actual_Observation_Set(&observed_var_temp);
		find_observed_order(&pos_of_observed_var, observed_var_temp, *comb_var_order);

		float temp;
		list<float> beta_total;
		auto it = this->Model_handlers.begin();
		for (it; it != this->Model_handlers.end(); it++)
			beta_total.push_back(0.f);
		list<size_t> obsv;

		auto it_beta = beta_total.begin();
		for (auto itL = comb_train_set->begin(); itL != comb_train_set->end(); itL++) {
			extract_observations(&obsv, *itL, pos_of_observed_var);
			this->Set_Observation_Set_val(obsv);
			this->Belief_Propagation(true);

			it = this->Model_handlers.begin();
			for (it_beta = beta_total.begin(); it_beta != beta_total.end(); it_beta++) {
				(*it)->Get_grad_beta_part(&temp);
				*it_beta += temp;
				it++;
			}
		}

		auto it_grad = grad_w->begin();
		temp = 1.f / (float)comb_train_set->size();
		for (it_beta = beta_total.begin(); it_beta != beta_total.end(); it_beta++) {
			*it_beta *= temp;
			*it_grad -= *it_beta;

			it_grad++;
		}

	}

	void Conditional_Random_Field::Get_Likelihood_estimation(float* result, std::list<size_t*>* comb_train_set, std::list<Categoric_var*>* comb_var_order) {

		list<size_t> pos_of_observed_var;
		list<Categoric_var*> observed_var_temp;
		this->Get_Actual_Observation_Set(&observed_var_temp);
		find_observed_order(&pos_of_observed_var, observed_var_temp, *comb_var_order);
		list<size_t> obsv;

		list<size_t> Y_MAP;
		list<size_t>::iterator it_temp;
		list<Categoric_var*> MAP_var_order;
		this->Get_Actual_Hidden_Set(&MAP_var_order);
		for (auto it_ob = observed_var_temp.begin(); it_ob != observed_var_temp.end(); it_ob++)
			MAP_var_order.push_back(*it_ob);
		size_t* Y_MAP_malloc = (size_t*)malloc(sizeof(size_t)* (MAP_var_order.size() + observed_var_temp.size()));

 		size_t k;
		float temp;
		*result = 0.f;
		for (auto it_set = comb_train_set->begin(); it_set != comb_train_set->end(); it_set++) {
			extract_observations(&obsv, *it_set, pos_of_observed_var);
			this->Set_Observation_Set_val(obsv);

			this->MAP_on_Hidden_set(&Y_MAP);
			k = 0;
			for (it_temp = Y_MAP.begin(); it_temp != Y_MAP.end(); it_temp++) {
				Y_MAP_malloc[k] = *it_temp;
				k++;
			}
			for (it_temp = obsv.begin(); it_temp != obsv.end(); it_temp++) {
				Y_MAP_malloc[k] = *it_temp;
				k++;
			}

			this->Get_Log_activation(&temp, Y_MAP_malloc, &MAP_var_order);
			*result -= temp;


			this->Get_Log_activation(&temp, *it_set, comb_var_order);
			*result += temp;
		}
		free(Y_MAP_malloc);

	}

}