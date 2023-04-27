/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gjupy <gjupy@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/26 15:47:56 by gjupy             #+#    #+#             */
/*   Updated: 2023/04/27 15:55:04 by gjupy            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "client.hpp"
#include <string>
#include <vector>
#include <set>

class Channel {

	private:
		std::string					_name;
		std::string					_topic;
		std::set<Client*>			_invited;
		std::set<Client*>			_registered;

		bool	invite_only;
		bool	topic_restriciton;
		bool	key;
		bool	privilege;
		bool	user_limit;

	public:
		Channel();
		~Channel();

		bool	get_invite_only() const;
		bool	get_topic_restriciton() const;
		bool	get_key() const;
		bool	get_privilege() const;
		bool	get_user_limit() const;

		void	set_invite_only(bool value);
		void	set_topic_restriciton(bool value);
		void	set_key(bool value);
		void	set_privilege(bool value);
		void	set_user_limit(bool value);
};