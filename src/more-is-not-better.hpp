/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   more-is-not-better.hpp
 * Author: bernard
 *
 * Created on 13 janvier 2017, 15:41
 */

#ifndef MORE_IS_NOT_BETTER_HPP
#define MORE_IS_NOT_BETTER_HPP

#if defined(_with_calloc)
#define MALLOC(bytes)	calloc(bytes, 1)
#else
#define MALLOC(bytes)	malloc(bytes)
#endif

#endif /* MORE_IS_NOT_BETTER_HPP */

