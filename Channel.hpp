/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gjupy <gjupy@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/26 15:47:56 by gjupy             #+#    #+#             */
/*   Updated: 2023/05/03 12:27:19 by gjupy            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "client.hpp"
#include <string>
#include <vector>
#include <set>

enum e_op_privilege {
	give,
	take
};

class Client;

class Channel {

	private:
		std::string							_name;
		std::string							_topic;
		std::string							_key;
		std::set<std::string>				_operators;
		std::set<Client*>					_invited;
		std::set<Client*>					_registered;

		bool	_invite_only;
		bool	_topic_restriciton;
		bool	_key_needed;
		bool	_user_limit;

	public:
		Channel(const std::string&, std::string&, const std::string&);
		~Channel();
		Channel(const Channel& src);
		Channel& operator=(const Channel& rhs);

		bool	get_invite_only() const;
		bool	get_topic_restriciton() const;
		bool	get_key_needed() const;
		bool	get_privilege() const;
		bool	get_user_limit() const;

		const std::set<Client*>&			get_invited() const;
		const std::set<Client*>&			get_registered() const;
		const std::string&					get_key() const;
		const std::string&					get_name() const;
		const std::set<std::string>			get_operators() const;

		void	set_invite_only(bool value);
		void	set_topic_restriciton(bool value);
		void	set_key_needed(bool value);
		void	set_operator(std::string&, int);
		void	set_user_limit(bool value);

		void	set_registered(Client&);
		void	set_invited(Client&);
		void	set_key(const std::string&);
};
