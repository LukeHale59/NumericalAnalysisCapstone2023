#include <typeinfo>

#include "parse_requests.h"
#include "function_map.h"

const std::string EMPTY = "";
const size_t MAX_SIZE = 1024;

namespace pt = boost::property_tree;


/**
 * @brief Once the service handles the client and receives the data, parse the json to extract
 * the function id, the matrix data, and etc.
 * 
 * @param receivedData 
 * @param bytes 
 * @return true 
 * @return false 
 */
result_t Capstone::parse_request(std::string receivedData, std::size_t bytes) {
    std::cout << " parse_request called\n" << std::endl;
    result_t result;
    std::string jsonData = extract_json(receivedData);

    if (jsonData == EMPTY) {
        result.succeeded = false;
        result.client_response = "Error: Ill formed JSON, cannot properly extract\n";
        return result;
    }

    std::cout << jsonData << std::endl;

    std::istringstream jsonStream(jsonData);
    pt::ptree jsonTree;
    pt::read_json(jsonStream, jsonTree);

    received_t msg;
    msg.func_id = jsonTree.get<std::string>("operation"); 
    /*  EXPECTED RESPONSE
     _______________
    |  Scalar --> 0 |
    |  Vector --> 1 |
    |  Matrix --> 2 |
    |_______________|
    */
    /* msg.exp_resp = jsonTree.get<std::string>("exp_resp");  */
    msg.data = jsonTree.get<std::string>("data");
    
    uint32_t id = std::stoi(msg.func_id); /* convert from string to uint32_t */
    std::tuple<std::vector<double>,std::vector<std::vector<double>>, std::vector<matrix>> data = Capstone::parse_data(msg); /* data tuple */
    /* uint32_t exp_resp = std::stoul(msg.exp_resp); */
    Capstone::map_func(id, data, &result);
    return result;
}

/**
 * @brief Since the client sends over more than just JSON data, need to extract the JSON and return it so that Boost throws
 * no errors
 * 
 * @param receivedData 
 * @return std::string 
 */
std::string Capstone::extract_json(std::string receivedData) {
    std::cout << " extract_json called\n" << std::endl;
    size_t startPos = receivedData.find('{');
    
    if (startPos != std::string::npos) { 
        size_t endPos = receivedData.find('}', startPos);

        if (endPos != std::string::npos) { 
            size_t length = endPos - startPos - 1;
            std::string result = receivedData.substr(startPos, length + 2);

            return result;
        } else {
            std::cout << "Matching '}' not found." << std::endl;
        }
    } else {
        std::cout << "'{' not found." << std::endl;
    }

    return EMPTY;
}

std::vector<double> Capstone::extract_vector(std::string vec_str){
    std::cout << " extract vector called\n" << std::endl;
    std::cout << vec_str << std::endl;
    std::vector<double> result;
    result.reserve(vec_str.length() / 2);
    std::istringstream ss(vec_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::cout << "Token: " << token << std::endl;
        double num = std::stod(token);
        result.push_back(num);
    }
    return result;
}

matrix Capstone::extract_matrix(std::string mat_str){
    std::cout << " extract matrix called\n" << std::endl;
    std::cout << mat_str << std::endl;
    size_t newline = mat_str.find('\n');
    /* extract dimensions without newline */
    std::string dim = mat_str.substr(0, newline); 
    /* extract matrix data without the newline */
    std::string data = mat_str.substr(newline + 1, mat_str.length() - (newline + 1));

    int num_rows, num_cols = 0;
    /* spliting the dimensions of variable length */
    std::vector<double> dim_vec = Capstone::extract_vector(dim);
    num_rows = (int) dim_vec[0];
    num_cols = (int) dim_vec[1];

    std::vector<double> data_vec = Capstone::extract_vector(data);

    matrix result(num_rows, std::vector<double>(num_cols));

    for(int row = 0; row < num_rows; row++){
        for(int col = 0; col < num_cols; col++){
            result[row][col] = data_vec[row * num_cols + col];
        }
    }
    return result;
}


std::tuple<std::vector<double>,std::vector<std::vector<double>>, std::vector<matrix>> Capstone::parse_data(received_t msg) {
    std::cout << " parse_data called\n" << std::endl;
    size_t d_pos = 0;
    std::string data = msg.data;

    std::vector<double> scalars;
    std::vector<std::vector<double>> vectors; 
    std::vector<matrix> matrices;

    while((d_pos = data.find(MAGIC_NUMBER)) != std::string::npos){
        std::cout << "PASS: " + data << std::endl;
        std::string obj = data.substr(0, d_pos);
        if(obj.length() == 1){ /* it is a scalar */
            scalars.push_back(std::stod(obj));
            data.erase(0, d_pos + MAGIC_NUMBER.length());
        }else if(obj.find('\n') == std::string::npos){ /* it is a vector */
            vectors.push_back(Capstone::extract_vector(obj));
            data.erase(0, d_pos + MAGIC_NUMBER.length());
        }else{ /* it is a matrix */
            matrices.push_back(Capstone::extract_matrix(obj));
            data.erase(0, d_pos + MAGIC_NUMBER.length());
        }
    }
    return std::make_tuple(scalars, vectors, matrices);
}

std::string Capstone::serialize_matrix(matrix mat){
    std::cout << " serialize_matrix called\n" << std::endl;
    std::string result = "";
    int num_rows = mat.size();
    int num_cols = mat[0].size();
    result += std::to_string(num_rows) + ", " + std::to_string(num_cols) + '\n';
    /* converting the matrix type to a string */
    for(int row = 0; row < (int) num_rows; row++){
        for(int col = 0; col < (int) num_cols; col++){
            result += std::to_string(mat[row][col]);
            if(row != num_rows - 1 || col != num_cols - 1){ /* only add comma if not the end */
                result += ", ";
            }
        }
    }
    return result + '\n'; /* add newline to the end */
    std::cout << "Result: " << result << std::endl;
}

std::string Capstone::serialize_vector(std::vector<double> vec){
    std::cout << " serialize_vector called\n" << std::endl;
    std::string result = "";
    for(int i = 0; i < (int) vec.size(); i ++){
        result += std::to_string(vec[i]);
        if(i != (int) vec.size() - 1){ /* only add comma if not the end */
            result += ", ";
        }
    }
    return result;
}

void Capstone::map_func(uint32_t id, std::tuple<std::vector<double>,std::vector<std::vector<double>>, std::vector<matrix>> data, result_t *resp){
    std::cout << "map_func called\n" << std::endl;
    switch(id){
        case 0x10:{ 
            std::vector<matrix> mat_list = std::get<2>(data); /* access the list of matrices from tuple */
            matrix m1 = mat_list[0];
            matrix m2 = mat_list[1];
            std::cout << "Matrix 1: " << Capstone::serialize_matrix(m1) << std::endl;
            std::cout << "Matrix 2: " << Capstone::serialize_matrix(m2) << std::endl;
            matrix sum = sum_matrix(m1, m2);
            std::string result = Capstone::serialize_matrix(sum);
            std::cout << "Result: " << result << std::endl;
            resp->client_response = result; /* attach the sum to the result struct in other scope */
            break;
        }
        case 0x11:{ /* multiply */

            break;
        }
        case 0x12:{ /* transpose */

            break;
        }
        default:
            resp->client_response = "Error generating function mapping\n";
            break;
    }
}


